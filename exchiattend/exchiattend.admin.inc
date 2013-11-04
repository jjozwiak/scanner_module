<?php
// $Id$

/**
 * @file
 * Administration page callbacks for the exacttarget_ga module.
 */

/**
 * Form builder. Configure annotations.
 *
 * @ingroup forms
 * @see system_settings_form().
 */
function exchiattend_admin_settings() {	


  $form['markup1'] = array(
	'#type' => 'markup',
    '#prefix' => '<div>',
    '#value' => t('The form below allows a user to upload a csv file that will update civiCRM contacts status for events from registered to attended.'),
    '#suffix' => '</div>',
  );

  $form['#attributes'] = array(
  	'enctype' => 'multipart/form-data'
  );

  $form['upload'] = array(
  	'#type' => 'file',
	'#title' => t('File upload'),
  );

  $form['submit'] = array(
    '#type' => 'submit', 
    '#default_value' => 'Upload',
  );

  return $form;

}

/*function exchiattend_settings_validate($form, $form_state) {
  if(!file_check_upload('upload')) {
    // If you want to require it, you'll want to do it here... something like this:
    form_set_error('upload', 'File missing for upload.');
  }
}

function exchiattend_admin_settings_submit($form,$form_state) {
  $file = file_check_upload('upload');
  //handle the file, using file_save_upload, or something similar
}*/