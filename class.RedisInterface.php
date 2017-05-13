<?php
/**
 * Interface Representing Redis API
 * This is helper for Various IDEs
 */
class Redis {
	public function __construct() {}
	/**
	 * @param string $host can be a host, or the path to a unix domain socket
	 * @param int $port optional
	 * @param float $timeout value in seconds (optional, default is 0 meaning unlimited)
	 */
	public function connect($host = "127.0.0.1", $port = 6379, $timeout = 0.0) {}

	/**
	 * Authenticate the connection using a password. Warning: The password is sent in plain-text over the network.
	 * @param string $password
	 * @return bool TRUE if the connection is authenticated, FALSE otherwise.
	 */
	public function auth($password) {}

	/**
	 * Remove specified keys.
	 * @param array $keys An array of keys, or an undefined number of parameters, each a key: key1 key2 key3 ... keyN
	 * @return bool
	 */
	public function delete($keys) {}

	/**
	 * Verify if the specified key exists.
	 * @param $key
	 * @return bool If the key exists, return TRUE, otherwise return FALSE.
	 */
	public function exists($key) {}

	/**
	 * Get the value related to the specified key
	 * @param $key
	 * @return bool|string If key didn't exist, FALSE is returned. Otherwise, the value related to this key is returned.
	 */
	public function get($key) {}

	/**
	 * Set the string value in argument as value of the key.
	 * @param string $key
	 * @param $value
	 * @param float $timeout Timeout (optional). Calling SETEX is preferred if you want a timeout.
	 * @return bool TRUE if the command is successful.
	 */
	public function set($key, $value, $timeout = 0.0) {}
}