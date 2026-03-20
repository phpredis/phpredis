extern "C" {
#include "php.h"
}

#include "redis_simdjson.h"

#include <simdjson.h>

#include <string>
#include <cstring>
#include <limits>

struct redis_zval {};

static thread_local simdjson::ondemand::parser tl_parser;
static thread_local std::string tl_scratch;

static inline const char *pad_json(const char *json, size_t len) {
    const size_t need = len + simdjson::SIMDJSON_PADDING;
    if (tl_scratch.size() < need) {
        tl_scratch.resize(need);
    }
    memcpy(tl_scratch.data(), json, len);
    memset(tl_scratch.data() + len, 0, simdjson::SIMDJSON_PADDING);
    return tl_scratch.data();
}

static
simdjson::error_code value_to_zval(zval *dst, simdjson::ondemand::value v,
                                   uint32_t flags, uint32_t depth);

static simdjson::error_code
array_to_zval(zval *dst, simdjson::ondemand::array a, uint32_t flags,
              uint32_t depth)
{
    if (depth == 0) {
        return simdjson::DEPTH_ERROR;
    }

    array_init(dst);

    for (auto elem : a) {
        simdjson::error_code err;
        zval zv;

        ZVAL_UNDEF(&zv);

        err = value_to_zval(&zv, elem.value(), flags, depth - 1);
        if (err) {
            zval_ptr_dtor(dst);
            ZVAL_UNDEF(dst);
            return err;
        }

        add_next_index_zval(dst, &zv);
    }

    return simdjson::SUCCESS;
}

static simdjson::error_code
object_to_zval(zval *dst, simdjson::ondemand::object o, uint32_t flags,
               uint32_t depth)
{
    if (depth == 0) {
        return simdjson::DEPTH_ERROR;
    }

    const bool assoc = !(flags & RJ_OBJECT);

    if (assoc) {
        array_init(dst);
    } else {
        object_init(dst);
    }

    for (auto field : o) {
        simdjson::error_code err;
        std::string_view ksv;
        zval zv;

        ZVAL_UNDEF(&zv);

        err = field.unescaped_key().get(ksv);
        if (err)
            return err;

        err = value_to_zval(&zv, field.value(), flags, depth - 1);
        if (err) {
            zval_ptr_dtor(dst);
            ZVAL_UNDEF(dst);
            return err;
        }

        if (assoc) {
            add_assoc_zval_ex(dst, ksv.data(), (uint32_t)ksv.size(), &zv);
        } else {
            zend_string *zs = zend_string_init(ksv.data(), ksv.size(), 0);
            zend_update_property(Z_OBJCE_P(dst), Z_OBJ_P(dst),
                                 ZSTR_VAL(zs), ZSTR_LEN(zs), &zv);
            zend_string_release(zs);
            zval_ptr_dtor(&zv);
        }
    }

    return simdjson::SUCCESS;
}

static simdjson::error_code
number_to_zval(zval *dst, simdjson::ondemand::number n, uint32_t flags)
{
    if (n.is_int64()) {
        int64_t i = n.get_int64();

        if (sizeof(zend_long) == 8) {
            ZVAL_LONG(dst, i);
            return simdjson::SUCCESS;
        }

        if (i >= (int64_t)std::numeric_limits<zend_long>::min() &&
            i <= (int64_t)std::numeric_limits<zend_long>::max())
        {
            ZVAL_LONG(dst, i);
            return simdjson::SUCCESS;
        }

        if (flags & RJ_BIGINT_AS_STRING) {
            char buf[32];
            int nbytes = snprintf(buf, sizeof(buf), "%lld", (long long)i);
            ZVAL_STRINGL(dst, buf, nbytes);
            return simdjson::SUCCESS;
        }

        ZVAL_DOUBLE(dst, (double)i);
        return simdjson::SUCCESS;
    }

    if (n.is_uint64()) {
        uint64_t u = n.get_uint64();

        if (sizeof(zend_long) == 8 &&
            u <= (uint64_t)std::numeric_limits<zend_long>::max()) {
            ZVAL_LONG(dst, (zend_long)u);
            return simdjson::SUCCESS;
        }

        if (flags & RJ_BIGINT_AS_STRING) {
            char buf[32];
            int nbytes = snprintf(buf, sizeof(buf), "%llu",
                                  (unsigned long long)u);
            ZVAL_STRINGL(dst, buf, (size_t)nbytes);
            return simdjson::SUCCESS;
        }

        ZVAL_DOUBLE(dst, (double)u);
        return simdjson::SUCCESS;
    }

    double d = n.get_double();

    ZVAL_DOUBLE(dst, d);
    return simdjson::SUCCESS;
}

template <typename T>
static simdjson::error_code
any_to_zval(zval *dst, T &src, uint32_t flags, uint32_t depth)
{
    using simdjson::ondemand::json_type;

    simdjson::error_code err;
    json_type t;

    err = src.type().get(t);
    if (err)
        return err;

    switch (t) {
        case json_type::null:
            ZVAL_NULL(dst);
            return simdjson::SUCCESS;

        case json_type::boolean: {
            bool b;
            err = src.get_bool().get(b);
            if (err)
                return err;
            ZVAL_BOOL(dst, b);
            return simdjson::SUCCESS;
        }

        case json_type::number: {
            simdjson::ondemand::number n;
            err = src.get_number().get(n);
            if (err)
                return err;
            return number_to_zval(dst, n, flags);
        }

        case json_type::string: {
            std::string_view sv;
            err = src.get_string().get(sv);
            if (err)
                return err;
            ZVAL_STRINGL(dst, sv.data(), sv.size());
            return simdjson::SUCCESS;
        }

        case json_type::array: {
            simdjson::ondemand::array a;
            err = src.get_array().get(a);
            if (err)
                return err;
            return array_to_zval(dst, a, flags, depth);
        }

        case json_type::object: {
            simdjson::ondemand::object o;
            err = src.get_object().get(o);
            if (err)
                return err;
            return object_to_zval(dst, o, flags, depth);
        }

        case json_type::unknown:
            return simdjson::INCORRECT_TYPE;
    }

    ZEND_UNREACHABLE();
}

static simdjson::error_code
document_to_zval(zval *dst, simdjson::ondemand::document &doc,
                 uint32_t flags, uint32_t depth)
{
    return any_to_zval(dst, doc, flags, depth);
}

static simdjson::error_code
value_to_zval(zval *dst, simdjson::ondemand::value v, uint32_t flags,
              uint32_t depth)
{
    return any_to_zval(dst, v, flags, depth);
}

static void redis_json_emit_error(int err) {
    if (err == 0)
        return;

    if (err < 0)
        err = -err;

    if (err < 0 || err > simdjson::NUM_ERROR_CODES) {
        php_error_docref(NULL, E_WARNING, "Unknown error code: %d", err);
        return;
    }

    simdjson::error_code code = static_cast<simdjson::error_code>(err);

    php_error_docref(NULL, E_WARNING, "Error parsing JSON: %s",
                     simdjson::error_message(code));
}

extern "C" int
redis_json_to_zval_ex(redis_zval *dst_, const char *json, size_t len,
                      uint32_t flags, uint32_t max_depth)
{
    auto *dst = reinterpret_cast<zval *>(dst_);
    simdjson::error_code err = simdjson::SUCCESS;

    if (max_depth == 0) {
        max_depth = 512;
    }

    const char *padded = pad_json(json, len);

    auto doc_res = tl_parser.iterate(padded, len, tl_scratch.size());

    err = doc_res.error();
    if (err) {
        ZVAL_NULL(dst);
        redis_json_emit_error(err);
        return -err;
    }

    simdjson::ondemand::document doc = std::move(doc_res).value_unsafe();

    err = document_to_zval(dst, doc, flags, max_depth);
    if (err) {
        ZVAL_NULL(dst);
        redis_json_emit_error(err);
        return -err;
    }

    return 0;
}
