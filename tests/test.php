<?php

// phpunit is such a pain to install, we're going with pure-PHP here.
class TestSuite {

	public static $errors = array();

	protected function assertFalse($bool) {
		$this->assertTrue(!$bool);
	}

	protected function assertTrue($bool) {
		if($bool)
			return;

		$bt = debug_backtrace(false);
		$count = count($bt);
		self::$errors []= sprintf("Assertion failed: %s:%d (%s)\n",
			$bt[$count - 2]["file"], $bt[$count - 3]["line"], $bt[$count - 1]["function"]);
	}

	protected function assertEquals($a, $b) {
		if($a === $b)
			return;

		$bt = debug_backtrace(false);
		$count = count($bt);
		self::$errors []= sprintf("Assertion failed (%s !== %s): %s:%d (%s)\n",
			print_r($a, true), print_r($b, true),
			$bt[$count - 2]["file"], $bt[$count - 3]["line"], $bt[$count - 1]["function"]);
	}

	public static function run($className) {

		$rc = new ReflectionClass($className);
		$methods = $rc->GetMethods(ReflectionMethod::IS_PUBLIC);

		foreach($methods as $m) {

			$name = $m->name;
			if(substr($name, 0, 4) !== 'test')
				continue;

			$count = count($className::$errors);
			$rt = new $className;
			$rt->setUp();
			$rt->$name();
			echo ($count === count($className::$errors)) ? "." : "F";
		}
		echo "\n";

		if(empty($className::$errors)) {
			echo "All tests passed.\n";
		}

		echo implode('', $className::$errors);
	}
}

?>
