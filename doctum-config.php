<?php

use Doctum\Doctum;
use Doctum\Version\GitVersionCollection;
use Doctum\RemoteRepository\GitHubRemoteRepository;

use Symfony\Component\Finder\Finder;

$root = realpath(__DIR__);

$iterator = Finder::create()
    ->files()
    ->name('*.stub.php')
    ->in($root);

//$versions = GitVersionCollection::create($root)
//    ->add('develop', 'develop');

return new Doctum($iterator, [
    'title' => 'PhpRedis API',
    'language' => 'en',
    'source_dir' => $root,
    'build_dir' => "{$root}/docs",
    'cache_dir' => "{$root}/docs/.cache",
    'base_url' => 'https://phpredis.github.io/',
//    'versions' => $versions,
    'remote_repository' => new GitHubRemoteRepository('phpredis/phpredis', $root),
]);
