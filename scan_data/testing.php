<?php
//Boostrap Drupal in standalone script file to access drupal functions
define('DRUPAL_ROOT', getcwd());
require_once './includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);


db_query("INSERT INTO {drupal_batchtest} (test, test2) VALUES (%s, %s)", 'batchtest', 'tttee');