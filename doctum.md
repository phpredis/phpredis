# API Documentation

```bash
curl -O https://doctum.long-term.support/releases/latest/doctum.phar
chmod +x doctum.phar

./doctum.phar update doctum-config.php
```

The build uses a custom Doctum theme stored in `doctum-theme/` so that extra
assets such as highlight.js are bundled with the generated HTML. Keep that
directory in sync if you ever update Doctum upstream assets.
