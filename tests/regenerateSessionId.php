<?php
error_reporting(E_ERROR | E_WARNING);

$redisHost = $argv[1];
$sessionId = $argv[2];
$locking = !!$argv[3];
$destroyPrevious = !!$argv[4];
$sessionProxy = !!$argv[5];

if (empty($redisHost)) {
    $redisHost = 'localhost';
}

ini_set('session.save_handler', 'redis');
ini_set('session.save_path', 'tcp://' . $redisHost . ':6379');

if ($locking) {
    ini_set('redis.session.locking_enabled', true);
}

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

if ($sessionProxy) {
    $handler = new TestHandler();
    session_set_save_handler($handler);
}

session_id($sessionId);
if (!session_start()) {
    $result = "FAILED: session_start()";
}
elseif (!session_regenerate_id($destroyPrevious)) {
    $result = "FAILED: session_regenerate_id()";
}
else {
    $result = session_id();
}
session_write_close();
echo $result;

