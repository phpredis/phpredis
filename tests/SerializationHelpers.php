<?php

/* Named subclasses let serialization hooks restore native state explicitly. */
trait RedisSerializationConstructor {
    private array $constructorArgs;

    public function __construct(...$args) {
        $this->constructorArgs = $args;
        parent::__construct(...$args);
    }

    public function __serialize(): array {
        return $this->constructorArgs;
    }

    public function __unserialize(array $args): void {
        $this->__construct(...$args);
    }
}

class SerializableRedis extends Redis {
    use RedisSerializationConstructor;
}

class SerializableRedisArray extends RedisArray {
    use RedisSerializationConstructor;
}

class SerializableRedisCluster extends RedisCluster {
    use RedisSerializationConstructor;
}

class SerializableRedisSentinel extends RedisSentinel {
    use RedisSerializationConstructor;
}
