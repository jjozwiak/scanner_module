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
function exchivip_admin_settings() {	


  $form['markup1'] = array(
	'#type' => 'markup',
    '#prefix' => '<div>',
    '#value' => t('The button below will look at all the current users in CiviCRM and create a unique key for all the users. It will then add a custom URL which they can go to and provide information and create a user account.'),
    '#suffix' => '</div>',
  );

  $form['submit'] = array(
    '#type' => 'submit', 
    '#default_value' => 'Generate URLS',
  );

  return $form;

}
//Generates random string to use in the unique url for end users
function gen_uniqid() {
	$char[0] = "ABCDEFGHIJ";
	$char[1] = "KLMNPQRSTU";
	$char[2] = "VWXYZABCDE";
	$char[3] = "FGHIJKLMNP";
	$char[4] = "QRSTUVWXYZ";
	$arr = preg_split('//', time(), -1, PREG_SPLIT_NO_EMPTY);
	
	$aCnt = count($arr);
	for ($i = 0; $i < $aCnt - 6; $i++) {
		unset($arr[$i]);
	}
	
	$uniqid = "";
	foreach($arr as $val) {
		$str = $char[rand(0,4)];
		$uniqid .= $str{$val};
	}
	
	//Recursive call that checks to see if the result in the database exsists and if it doesn't then return	
	$query = "SELECT id FROM civicrm_value_unique_url_18 WHERE unique_id_61 = '".$uniqid."'";
	$results = mysql_query($query);
	
	if($results->num_row > 0){
		$uniqid = gen_uniqid();
	}else{
		return $uniqid;
	}
	
	
	return $uniqid;
}
function exchivip_admin_settings_submit($form,$form_state){
	
	watchdog('GEN_VIP_URL', 'START');
	
	//NOTE: Turns out there is a limit to the civicrm api queries.  See rowCount key below.
	//However, giving a high row count for this script exceeds the default max_execution_time of 30 in php.ini.
	//I increased this setting to 1000 and the script ran successfully through all contacts. Not sure if this is the
	//best option though. The offset key allows batching which may be an option worth exploring.
	
	//Set our paramaters for the contact get
	//custom_61: Unique ID
	//Custom_62: Unique URL
	$params = array( 
/*	'custom_61' => ' ', */
	'group' => 3,
  	'return' => 'custom_61,custom_62',
  	'version' => 3,
	// Increase row count if there are more VIPs
  	'rowCount' => 50000,
  	//'offset' => 80, //- get 5000 records starting @ record 80
	);

	//Returns our contact requests
  	$result = civicrm_api( 'contact','get',$params );
	watchdog('GEN_VIP_URL', '<pre>COUNT = '. print_r($result['count'], TRUE) .'</pre>');
	if ($result['is_error'] != 0) {
		watchdog('GEN_VIP_URL - err', '<pre>SEARCH-result = '. print_r($result, TRUE) .'<br />params = '.print_r($params, TRUE).'</pre>');
	}
	
	//Loop through all results, this will take awhile
	$wdog = 0;
	
	foreach($result['values'] as $id => $values){
		
		//If the Unique ID is already set then don't set another one
		if((!isset($values['custom_61']) || $values['custom_61'] == '') && $values['custom_61'] != 'active'){
			
			//check to make sure contact is in a VIP group before assigning a unique URL
			$query = db_query("SELECT civicrm_group_contact.contact_id FROM civicrm_group_contact WHERE contact_id ='%s' AND status = 'Added' AND (group_id = 4 OR group_id = 5)",$id);
			
			if(mysql_num_rows($query) > 0){
				
				//Get our random string
				$random_string = gen_uniqid();
				//Create our URL based off the current host
				$url = $_SERVER['HTTP_HOST'].'/vip/'.$random_string;
				//Update the user in CiviCRM
				if ($wdog < 2) {
					watchdog('GEN_VIP_URL', '<pre>random_string = '. $random_string .'<br />url='.$url.'<br />id='.$id.'</pre>');
					watchdog('GEN_VIP_URL', '<pre>VALUE = '. print_r($values, TRUE) .'</pre>');
					$wdog++;
				}
				if (!exchivip_update(array('id'=>$id,'custom_62'=>$url,'custom_61'=>$random_string))) {
					break;
				}
			}			
		}
	}
	watchdog('GEN_VIP_URL', 'END');
}

function exchivip_update($params = array()){

	$params['version'] = '3';
	$params['debug'] = '1';
	//pass array to update function
	$result = civicrm_api("Contact", "create", $params);
	if ($result['is_error'] != 0) {
		watchdog('GEN_VIP_URL - err', '<pre>UPDATE-result = '. print_r($result, TRUE) .'<br />params = '.print_r($params, TRUE).'</pre>');
		return false;
	}
	else {
		return true;
	}
}
