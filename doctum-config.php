<?php

use Doctum\Doctum;
use Doctum\RemoteRepository\GitHubRemoteRepository;
use Symfony\Component\Finder\Finder;

$root = realpath(__DIR__);
Doctum::$defaultVersionName = 'develop';

$iterator = Finder::create()
    ->files()
    ->name('*.stub.php')
    ->in($root);

return new Doctum($iterator, [
    'title' => 'PhpRedis API',
    'language' => 'en',
    'source_dir' => $root,
    'build_dir' => $root . '/docs',
    'cache_dir' => $root . '/docs/.cache',
    'theme' => 'phpredis',
    'template_dirs' => [
        $root . '/doctum-theme',
    ],
    'base_url' => 'https://phpredis.github.io/',
    'remote_repository' => new GitHubRemoteRepository(
        'phpredis/phpredis',
        $root
    ),
]);
