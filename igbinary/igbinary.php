<?php
/**
 * Igbinary is a drop in replacement for the standard php serializer.
 * Instead of time and space consuming textual presentation, igbinary stores php data
 * structures in a compact binary form. Savings are significant when using
 * memcached or similar memory based storages for serialized data.
 *
 * But where does the name "igbinary" come from? There was once a similar project
 * called fbinary but it has disappeared from the Internet. Its architecture
 * wasn't very clean either. IG is short name for a finnish social networking site
 * {@link http://irc-galleria.net/ IRC-Galleria}.
 *
 * Storing complex php data structures like arrays of associative arrays in
 * serialized form is not very space efficient. Igbinary uses two strategies to
 * to minimize size of the serialized form.
 *
 * Strings are stored only once by using a hash table. Arrays of associate arrays
 * with very verbose keys are stored very compactly.
 *
 * Numerical values are stored in the smallest primitive data type available.
 * 123 = int8_t,
 * 1234 = int16_t,
 * 123456 = int32_t
 * ... and so on.
 *
 * This file is igbinary phpdoc documentation stub.
 *
 * @author Oleg Grenrus <oleg.grenrus@dynamoid.com>
 * @version 1.0.0
 * @package igbinary
 */

/** Generates a storable representation of a value.
 * This is useful for storing or passing PHP values around without losing their type and structure.
 * To make the serialized string into a PHP value again, use {@link igbinary_unserialize}.
 *
 * igbinary_serialize() handles all types, except the resource-type.
 * You can even serialize() arrays that contain references to itself.
 * Circular references inside the array/object you are serialize()ing will also be stored.
 *
 * If object implements {@link http://www.php.net/~helly/php/ext/spl/interfaceSerializable.html Serializable} -interface,
 * PHP will call the member function serialize to get serialized representation of object.
 *
 * When serializing objects, PHP will attempt to call the member function __sleep prior to serialization.
 * This is to allow the object to do any last minute clean-up, etc. prior to being serialized.
 * Likewise, when the object is restored using unserialize() the __wakeup member function is called.
 *
 * @param mixed $value The value to be serialized.
 * @return string Returns a string containing a byte-stream representation of value that can be stored anywhere.
 * @link http://www.php.net/serialize PHP default serialize
 */
function igbinary_serialize($value);

/** Creates a PHP value from a stored representation.
 * igbinary_unserialize() takes a single serialized variable and converts it back into a PHP value.
 *
 * If the variable being unserialized is an object, after successfully reconstructing the object
 * PHP will automatically attempt to call the __wakeup() member function (if it exists).
 * In case the passed string is not unserializeable, NULL is returned and E_WARNING is issued.
 *
 * @param string $str The serialized string.
 * @return mixed The converted value is returned, and can be a boolean, integer, float, string, array or object.
 * @link http://www.php.net/manual/en/function.unserialize.php PHP default unserialize
 * @link http://www.php.net/~helly/php/ext/spl/interfaceSerializable.html Serializable
 */
function igbinary_unserialize($str);

?>
