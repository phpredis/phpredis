<?php

error_reporting(E_ERROR | E_WARNING);

$opt = getopt('', [
    'handler:', 'save-path:', 'id:', 'locking-enabled:',
    'destroy-previous:', 'proxy:'
]);

$handler = $opt['handler'] ?? NULL;
$save_path = $opt['save-path'] ?? NULL;
$id = $opt['id'] ?? NULL;
$locking_enabled = !!($opt['locking-enabled'] ?? false);
$destroy_previous = !!($opt['destroy-previous'] ?? false);
$proxy = !!($opt['proxy'] ?? false);

if ( ! $handler) {
    fprintf(STDERR, "--handler is required\n");
    exit(1);
} else if ( ! $save_path) {
    fprintf(STDERR, "--save-path is required\n");
    exit(1);
}

ini_set('session.save_handler', $handler);
ini_set('session.save_path', $save_path);
ini_set('redis.session.locking_enabled', $locking_enabled);

if (interface_exists('SessionHandlerInterface')) {
    class TestHandler implements SessionHandlerInterface
    {
        /**
         * @var SessionHandler
         */
        private $handler;

        public function __construct()
        {
            $this->handler = new SessionHandler();
        }

        public function close()
        {
            return $this->handler->close();
        }

        public function destroy($session_id)
        {
            return $this->handler->destroy($session_id);
        }

        public function gc($maxlifetime)
        {
            return $this->handler->gc($maxlifetime);
        }

        public function open($save_path, $name)
        {
            return $this->handler->open($save_path, $name);
        }

        public function read($session_id)
        {
            return $this->handler->read($session_id);
        }

        public function write($session_id, $session_data)
        {
            return $this->handler->write($session_id, $session_data);
        }
    }
}

if ($proxy) {
    $handler = new TestHandler();
    session_set_save_handler($handler);
}

session_id($id);

if ( !  session_start()) {
    $result = "FAILED: session_start()";
} else if ( ! session_regenerate_id($destroy_previous)) {
    $result = "FAILED: session_regenerateId()";
} else {
    $result = session_id();
}

session_write_close();

echo $result;
