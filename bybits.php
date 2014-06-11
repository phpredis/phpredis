<?php
$arr = explode("\n",file_get_contents("redis.c"));
$idx = [];

foreach($arr as $str) {
    if(strpos($str, "zend_parse_method_parameters")!==false) {
        $start = strpos($str, '"O');
        $str = substr($str, $start+1);
        $end = strpos($str, '"');
        $str = substr($str, 0, $end);

        if(!isset($idx[$str])) {
            $idx[$str]=1;
        } else {
            $idx[$str]++;
        }
    }
}

arsort($idx);

foreach($idx as $proto => $count) {
    echo $proto . "\t" . $count . "\n";
}
?>
