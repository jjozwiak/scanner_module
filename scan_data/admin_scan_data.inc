<?php

function scan_data_import() {
	$output = "<h2>Scanner Data Import</h2><p>Use the form below to Imort a csv file of scanner data into CiviCRM</p>";
	$output .= drupal_get_form('scan_data_export_form');
	//$output .= "Export Form";
	return $output;
}

function scan_data_import_form(&$form_state){

	$result = db_query("SELECT scan_data_config.civicrm_table_name FROM scan_data_config WHERE active = 1");

	$options = array('-- Select Field --');
	$label = array();

	while ($field = db_fetch_array($result)) {

	    $title_result = db_query("SELECT civicrm_custom_group.title
	                        	FROM civicrm_custom_group
	                        	WHERE civicrm_custom_group.table_name = '%s'", $field['civicrm_table_name']);

	    while ($title = db_fetch_array($title_result)){
	      	$label = $title['title'];
	    }

	    $options[$field['civicrm_table_name']] = $label;
	}

	$form['#attributes'] = array('enctype' => "multipart/form-data");

	$form['civiFieldTable'] = array(
		'#type'     => 'select',
		'#title'    => 'Import Field',
  		'#options'  => $options,
		'#required' => TRUE
	);

	$form['file_upload'] = array(
		'#type' 	=> 'file',
		'#title' 	=> 'Attach scanner data'
	);

	$form['submit'] 	= array(
		'#type' 	=> 'submit',
		'#value' 	=> t('Upload')
	);
	// $form['#validate'] = array(
	//     'scan_data_validate_fileupload',
	//     'scan_data_form_validate',
	//  );
	return $form;
}

function scan_data_import_form_validate($form, &$form_state) {

	// attempt to save the uploaded file
	$file = file_save_upload('file_upload');
	// check file uploaded OK
	if (!$file) {
		form_set_error('file_upload', t('You must attach a file.'));
	}
	else if($file->filemime != 'text/csv') {
		form_set_error('file_upload', t('The file must be of CSV type only.'));
	}
	else {
	// set files to form_state, to process when form is submitted
		$form_state['values']['file_upload'] = $file;
	}
}

function scan_data_import_form_submit(&$form, &$form_state) {


	$batch = array(
	    'title' => t('Importing CSV ...'),
	    'operations' => array(),
	    'init_message' => t('Commencing'),
	    'progress_message' => t('Processed @current out of @total.'),
	    'error_message' => t('An error occurred during processing'),
	    'finished' => 'scan_data_import_finished',
  	);


  //Loop through csv and assign each line to an operation in the batch
  if ( isset( $form_state['values']['file_upload'] ) ) {

    if ( $handle = fopen($form_state['values']['file_upload'], 'r') ) {

      $line_count = 1 ;
      $first = TRUE ;
      $line = fgetcsv($handle, 4096);
      while ( $line = fgetcsv($handle, 4096) ) {
        /**
         * we use base64_encode to ensure we don't overload the batch
         * processor by stuffing complex objects into it
         */
        //$batch['operations'][] = array('_scan_data_import_line', array(array_map('base64_encode', $line)));
        $batch['operations'][] = array('_scan_data_import_line', $line);
      }
      fclose($handle);
    } // we caught this in csvimport_form_validate()
  } // we caught this in csvimport_form_validate()
  batch_set($batch);

}


/**
 * Remember the uploaded CSV filename
 *
 * @TODO is there a better way to pass a value from inception of the
 * batch to the finished function?
 */
function _scan_data_remember_filename($filename, &$context) {
  $context['results']['uploaded_filename'] = $filename ;
}



/**
 * Process a single line.
 */
function _scan_data_import_line($line, &$context) {
	db_query("INSERT INTO {batchtest} (test, test2) VALUES ('%s', '%s')", 'batchtest', 'tttee');
}

function scan_data_import_finished($success, $results, $operations) {
  if ( !empty($results['failed_rows']) ) {
    $dir  = file_directory_path() . '/csvimport/' ;
    if ( file_check_directory( $dir, FILE_CREATE_DIRECTORY ) ) {
      $csv_filename = 'failed_rows-'. basename($results['uploaded_filename']); // we validated extension on upload
      $csv_filepath = $dir .'/'. $csv_filename;
      $targs = array(
        '!csv_url' => l(check_plain($csv_filename), $csv_filepath),
        '%csv_filename' => $csv_filename,
        '%csv_filepath' => $csv_filepath,
      ) ;
      if ( $handle = fopen($csv_filepath, 'w+') ) {
        foreach( $results['failed_rows'] as $failed_row ) {
          fputcsv($handle, $failed_row);
        }
        fclose($handle);
        drupal_set_message(t('Some rows failed to import. You may download a CSV of these rows: !csv_url', $targs), 'error');
      }
      else {
        drupal_set_message(t('Some rows failed to import, but unable to write error CSV to %csv_filepath', $targs), 'error');
      }
    }
    else {
      drupal_set_message(t('Some rows failed to import, but unable to create directory for error CSV at %csv_directory', $targs), 'error');
    }
  }
  return t('The CSV import has completed.');
}


function scan_data_export() {
	$output = "<h2>" . t('Scanner Data Export') . "</h2>";
	$output .= drupal_get_form('scan_data_export_form');

	//get column name
	$table = $_GET['civiExportFieldTable'];
	if($table != ""){
	    $tablequery = db_query("SELECT civicrm_custom_field.column_name
	    						FROM civicrm_custom_field INNER JOIN civicrm_custom_group
	    						ON civicrm_custom_field.custom_group_id = civicrm_custom_group.id
	    						WHERE civicrm_custom_group.table_name = '$table'");

	    $column_name = array();
	    while($row = db_fetch_array($tablequery)){
	      $column_name[] = $row['column_name'];
	    }

	    $searchTable = $_GET['civiExportFieldTable'];
	    $searchTableField = $searchTable . '.entity_id';

	    //take the to and from values from the form and fromat them to mySQL date format
	    $fromDate = $_GET['dateFrom']['year'] . "-" . $_GET['dateFrom']['month'] . "-" . $_GET['dateFrom']['day'] . " 00:00:00";
	    $toDate = $_GET['dateTo']['year'] . "-" . $_GET['dateTo']['month'] . "-" . $_GET['dateTo']['day'] . " 00:00:00";

	    $result = db_query("SELECT *
	                        FROM civicrm_contact
	                        INNER JOIN %s
	                        ON civicrm_contact.id = %s
	                        WHERE " . $searchTable . "." . $column_name[0] . " > '" . $fromDate . "'
	                        AND " . $searchTable . "." . $column_name[0] . " < '" . $toDate . "'", $searchTable, $searchTableField
	                        );

    	$results = array();

	    while ($row = db_fetch_array($result)) {

		    $address_lookup = db_query("SELECT civicrm_state_province.`abbreviation`,
		                                civicrm_address.street_address,
		                                civicrm_address.city,
		                                civicrm_country.`name`,
		                                civicrm_contact.display_name,
		                                civicrm_address.supplemental_address_1,
		                                civicrm_address.postal_code
		                                FROM civicrm_address INNER JOIN civicrm_state_province ON civicrm_address.state_province_id = civicrm_state_province.id
		                                INNER JOIN civicrm_country ON civicrm_address.country_id = civicrm_country.id
		                                INNER JOIN civicrm_contact ON civicrm_address.contact_id = civicrm_contact.id
		                                WHERE civicrm_address.contact_id = %s", $row['entity_id']);

		    $address_lookup_result = db_fetch_array($address_lookup);

	        //!TODO Make sure this pulls the VIP field only (i.e. get the table name from Expo's database and query that table only)
	        $vip_lookup = db_query("SELECT *
	                                FROM civicrm_group_contact
	                                INNER JOIN civicrm_group
	                                ON civicrm_group_contact.group_id = civicrm_group.id
	                                WHERE civicrm_group_contact.contact_id = %s AND civicrm_group_contact.status = 'Added'", $row['entity_id']);

	        $vip_lookup_result = db_fetch_array($vip_lookup);

	        $results[] = array($row['entity_id'],
	                            $row['first_name'],
	                            $row['last_name'],
	                            $address_lookup_result['street_address'],
	                            $address_lookup_result['supplemental_address_1'],
	                            $address_lookup_result['city'],
	                            $address_lookup_result['abbreviation'],
	                            $address_lookup_result['postal_code'],
	                            $address_lookup_result['name'],
	                            $vip_lookup_result['title'],
	                            $row[$column_name[0]],
	                            $row[$column_name[1]],
	                            '<a href="/civicrm/contact/view?reset=1&cid=' . $row['entity_id'] . '">view</a>');
		}
	}

	$header = array('CiviCRM ID','First Name', 'Last Name','Address Line 1','Address Line 2','City','State','Zip','Country','VIP LEVEL', 'Dates', 'Location', 'CiviCRM Profile');

	if($_GET['csvExport'] == 1){

    	function create_csv_string($data) {

			// Open temp file pointer
			if (!$fp = fopen('php://temp', 'w+')) return FALSE;

			// Loop data and write to file pointer
			foreach ($data as $line) fputcsv($fp, $line);

			// Place stream pointer at beginning
			rewind($fp);

			// Return the data
			return stream_get_contents($fp);

    	}

	    function send_csv_mail ($csvData, $body, $to, $subject = 'Scanner Export File', $from = 'no-reply@expositionchicago.com') {

			// This will provide plenty adequate entropy
			$multipartSep = '-----'.md5(time()).'-----';

			// Arrays are much more readable
			$headers = array(
			"From: $from",
			"Reply-To: $from",
			"Content-Type: multipart/mixed; boundary=\"$multipartSep\""
			);

			// Make the attachment
			$attachment = chunk_split(base64_encode(create_csv_string($csvData)));

			// Make the body of the message
			$body = "--$multipartSep\r\n"
			    . "Content-Type: text/plain; charset=ISO-8859-1; format=flowed\r\n"
			    . "Content-Transfer-Encoding: 7bit\r\n"
			    . "\r\n"
			    . "$body\r\n"
			    . "--$multipartSep\r\n"
			    . "Content-Type: text/csv\r\n"
			    . "Content-Transfer-Encoding: base64\r\n"
			    . "Content-Disposition: attachment; filename=\"file.csv\"\r\n"
			    . "\r\n"
			    . "$attachment\r\n"
			    . "--$multipartSep--";

			// Send the email, return the result
			return @mail($to, $subject, $body, implode("\r\n", $headers));

	    }

		send_csv_mail($results, "Attached is scanner data from the " . $searchTable . " field group.", $_GET['email']);

	}

	//!TODO make sortable and paginate - http://ericlondon.com/posts/205-creating-a-table-with-sortable-columns-and-pagination-from-custom-sql

	$output .= theme('table', $header, $results);

	return $output;

}

function scan_data_export_form(&$form_state){

	$result = db_query("SELECT scan_data_config.civicrm_table_name FROM scan_data_config WHERE active = 1");

	//TODO add ability to export all fields

	$options = array();
	while ($field = db_fetch_array($result)) {

		//!TODO this is used in the import form as well. refactor to a function that both can use
		$title_result = db_query("SELECT civicrm_custom_group.title
		                    		FROM civicrm_custom_group
		                    		WHERE civicrm_custom_group.table_name = '%s'", $field['civicrm_table_name']);

		while ($title = db_fetch_array($title_result)){
			$label = $title['title'];
		}

		$options[$field['civicrm_table_name']] = $label;

	}
	$form['#method'] = 'GET';
	$form['civiExportFieldTable'] = array(
		'#type'     => 'select',
		'#title'    => t('Export Field'),
		'#options'  => $options,
		'#required' => TRUE,
	);

	$form['dateFrom'] = array(
		'#type'   => 'date',
		'#title'  => t('From')
	);

	$form['dateTo'] = array(
		'#type'   => 'date',
		'#title'  => t('To')
	);

	$form['csvExport'] = array(
		'#type'           => 'checkbox',
		'#title'          => t('Download to CSV?'),
		'#default_value'  => TRUE
	);

	$form['email'] = array(
		'#type'   => 'textfield',
		'#title'  => t('Email')
	);

	$form['submit'] = array(
		'#type'   => 'submit',
		'#value'  => t('Submit')
	);

	return $form;
}

function scan_data_config() {
	$custom_field_list .= "</ul>";
	$output = "<h2>Scanner Data Configuration</h2>";
	$output .= drupal_get_form('scan_data_config_form');
	return $output;
}

function scan_data_config_form(&$form_state){

	$custom_civicrm_fields = civicrm_api("CustomGroup","get", array (version => '3','sequential' =>'1'));

	$options = array();

	foreach ($custom_civicrm_fields['values'] as $field) {
		$options[$field['table_name']] = t($field['title']);
	}

	//default values array queries the scanner config table and returns values
	$result = db_query("SELECT scan_data_config.civicrm_table_name FROM scan_data_config WHERE active = 1");

	$default_values = array();

	while ($field = db_fetch_array($result)) {
		$default_values[] = $field['civicrm_table_name'];
	}

	$form['custom_fields'] = array(
		'#title' => 'CiviCRM Custom Fields',
		'#description' => 'Select which custom fields that you would like to make available for scanner data imports.',
		'#type' => 'checkboxes',
		'#options' => $options,
		'#default_value' => $default_values,
		'#required' => TRUE
	);

	$form['submit']   = array(
		'#type'   => 'submit',
		'#value'  => t('Update')
	);

    return $form;
}

function scan_data_config_form_submit($form, &$form_state) {
	$selection = $form_state['values']['custom_fields'];

	//reset all rows in config table to inactive
	db_query("UPDATE scan_data_config SET active = '%d'", 0);

  	//based on the checkboxes selected we will either add a new option or set a previously inactive option to active
	foreach($selection as $selected){
		//this logic might be more readable if we did an array comparison instead of nested if statements just a thought
		if($selected != NULL){

			$num_rows = db_result(db_query("SELECT * FROM scan_data_config WHERE civicrm_table_name = '%s'", $selected));

			if($num_rows == 0){
				db_query("INSERT INTO  scan_data_config (civicrm_table_name, active) VALUES ('%s', '%d')", $selected, 1);
			}else{
				db_query("UPDATE scan_data_config SET active = '%d' WHERE civicrm_table_name = '%s'", 1, $selected);
			}
		}
	}
	drupal_set_message(t("Fields Successfully Updated!"));
}