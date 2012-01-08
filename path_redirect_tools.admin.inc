<?php
/**
 * File path redirect tools module file.
 */

/**
 * Callback for deleting aliases.
 */
function path_redirect_tools_massdelete_form($form_state, $args_from_url = NULL) {
  if(isset($form_state['storage']['confirm'])) {
     // Final submit handler (after user clicks Yes, I Confirm).
    $form['super_confirm'] = array(
      '#type' => 'markup',
      '#value' => '<h2>Really?</h2>',
    );

    return confirm_form($form, t("Are you sure you know what you're doing?"), 'admin/build/path-redirect/tools/delete', t('I got this...'), t('Yes, nuke em all'), 'Cancel');
  }
  
  // do your normal $form definition here
  $form['confirmx'] = array(
    '#type' => 'markup',
    '#value' => '<p><strong>Are you <em>sure</em> you want to delete <em>all</em> redirects?</strong></p>',
  );

  $form['text'] = array(
    '#type' => 'submit',
    '#value' => 'Delete redirects',
    '#description' => 'bar',
  );

  return $form;
}

/**
 * Form validation callback for delete.
 */
function path_redirect_tools_massdelete_form_validate($form, &$form_state) {
  if(!isset($form_state['storage']['confirm'])) {
    // Put any initial validation here. This only be called the first time
    // through, not when the confirm button has been pushed.
  }
}

/**
 * Form submit callback for delete.
 */
function path_redirect_tools_massdelete_form_submit($form, &$form_state) {
  // Before confirm.
  if(!isset($form_state['storage']['confirm'])) {
    // This will cause the form to be rebuilt, entering the confirm part of the
    // form.
    $form_state['storage']['confirm'] = TRUE;
    // Along with this.
    $form_state['rebuild'] = TRUE;
  }
  // On confirm.
  else {
    // This is after they have pressed the confirm button.
    db_query('DELETE FROM {path_redirect} WHERE rid > 0');
    $num_rows = db_affected_rows();
    drupal_set_message($num_rows .' redirects have been deleted');
    drupal_goto('admin/build/path-redirect/tools/import');
  }
}

/**
 * Implements hook_admin_settings().
 */
function path_redirect_tools_admin_settings_form(&$form_state) {
  // Link specific configurations.
  $form['general_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('General Settings'),
    '#description' => t('Settings go here...'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );

  $form['general_settings']['path_redirect_tools_setting'] = array(
    '#type' => 'textfield',
    '#title' => t('No settings like home'),
    '#default_value' => variable_get('path_redirect_tools_setting', 'This setting doesn\'t do anything'),
    '#description' => t('Nothing, this setting does.')
  );

  return system_settings_form($form);
}

/**
 * Form callback to import redirects.
 */
function path_redirect_tools_form(&$form_state) {
  drupal_set_title('Import URL redirects');
  
  $form = array();
  
  $form['#attributes'] = array('enctype' => "multipart/form-data");
  
  $form['path_redirect_tools'] = array(
    '#type' => 'fieldset',
    '#title' => 'Instructions',
  );

  $form['path_redirect_tools']['instructions'] = array(
    '#type' => 'markup',
    '#title' => 'Instructions',
    '#value' => '<p>Import file must be formatted as follows including the column names in the first column:</p>' .
                '<pre>old_url,new_url,redirect_code<br />' .
                'my-old/url,new-url,301<br />' .
                'my-old/page,&lt;front&gt;,301' .
                '</pre><p><em>redirect_code</em> defaults to 301</p>',
  );

  $form['path_redirect_tools']['convert_to_system'] = array(
    '#type' => 'checkbox',
    '#title' => t('Convert to system URL'),
    '#description' => t('Convert a Drupal URL alias to its system URL equilvalent. This should normally be kept checked.'),
    '#default_value' => 1,
  );
  
  $form['source'] = array(
    '#type' => 'file',
    '#title' => t('File'),
    '#description' => t('CSV file for import.'),
  );
  
  $form['submit_test'] = array(
    '#type' => 'submit',
    '#value' => 'Test run',
    '#description' => t('This will simply examime the results and provide a report'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Import aliases',
  );
  
  return $form;
}

/**
 * Validate callback for imports.
 */
function path_redirect_tools_form_validate($form, &$form_state) {
  // if error
  // form_set_error('redirect', t('You are attempting to redirect the page to itself. This will result in an infinite loop.'));
}

/**
 * Submit handler for imports.
 */
function path_redirect_tools_form_submit($form, &$form_state) {

  // process the file
  $file = $_FILES['files']['tmp_name']['source'];

  switch ($form_state['clicked_button']['#value']) {
    case 'Test run':
      $output = path_redirect_tools_process_upload($file, FALSE);
      if ($output) {
        $header = array_shift($output);
        drupal_set_message(theme('table', $header, $output));
      }
      else {
        drupal_set_message('No output.');
      }
      break;

    case 'Import aliases':
      $output = path_redirect_tools_process_upload($file, TRUE);
      $msg = path_redirect_tools_process_import($output, $form_state);
      drupal_set_message($msg['added'] .' URL aliases were added, '. $msg['updated'] .' were updated');
       break;
  }
}

function path_redirect_tools_report_form(&$form_state) {
  $form = array();
  $form['fs'] = array(
    '#type' => 'fieldset',
    '#title' => t('URL alias report'),
  );
  $form['fs']['report_options'] = array(
    '#type' => 'radios',
    '#title' => t('Report options'),
    '#default_value' => $form_state['storage']['report_options'] ? $form_state['storage']['report_options'] : 'alias',
    '#options' => array(
      'alias' => 'Nodes to aliases',
      'source' => 'Aliases to nodes'),
    '#description' => t('<p><strong>Node to alias</strong>: return an alias for a given Drupal system path (if one exists).</p>' .
                        '<p><strong>Alias to nodes</strong>: return the Drupal system URL for a path alias (if one exists).</p>')
  );
  $form['fs']['combine_urls'] = array(
    '#type' => 'checkbox',
    '#title' => t('Combine URLs'),
    '#default_value' => $form_state['storage']['combine_urls'] ? $form_state['storage']['combine_urls'] : 0,
    '#description' => t('<p>This will export both URLs separated by comma. Uncheck to only display the alias / system URL.</p>')
  );
  $form['fs']['urls'] = array(
    '#type' => 'textarea',
    '#title' => t('Enter URLs to report on'),
    '#description' => t('Enter a list of URLs. The list should be in the format node/123 where 123 is the Node ID - or - a list of URL aliases to conver to node/nid format.'),
    '#rows' => 10,
    '#cols' => 50,
    '#default_value' => $form_state['storage']['urls'],
    '#required' => TRUE,
  );
  $form['fs']['submit'] = array(
    '#type' => 'submit',
    '#default_value' => t('Show me the URLs'),
  );
  if ($form_state['storage']['report']) {
    $form['report'] = array(
      '#type' => 'textarea',
      '#title' => t('URL report'),
      '#description' => t('This is where the data will be displayed.'),
      '#rows' => 10,
      '#cols' => 50,
      '#default_value' => $form_state['storage']['report'],
    );
  }

  return $form;
}

function path_redirect_tools_report_form_submit($form, &$form_state) {
  $cr = "\r\n";
  $urls = explode($cr, $form_state['values']['urls']);  
  $action = $form_state['values']['report_options'];
  $report = '';
  foreach ($urls as $url) {
    if (trim($url)) {
      $path = path_redirect_tools_convert_to_system($url, $action);
      if ($form_state['values']['combine_urls']) {
        $report .= $url . ',' . $path . $cr;
      }
      else {
        $report .= $path . $cr;
      }
    }
  }

  // Set the form values to their submitted state.
  $form_state['storage']['report'] = $report;
  $form_state['storage']['combine_urls'] = $form_state['values']['combine_urls'];
  $form_state['storage']['report_options'] = $form_state['values']['report_options'];
  $form_state['storage']['urls'] = $form_state['values']['urls'];
  $form_state['rebuild'] = TRUE;
}

/**
 * Process an array of url aliases.
 */
function path_redirect_tools_process_import($imports, &$form_state) {
  // default redirect settings
  $redirect = array(
    'rid' => NULL,
    'source' => '',
    'source_query' => array(),
    'redirect' => '',
    'query' => array(),
    'fragment' => '',
    'language' => '',
    'type' => variable_get('path_redirect_default_status', 301),
  );
  
  $added = $updated = 0;
  $redirects = array();
  // process each import
  foreach ($imports as $import) {
    $rdrct = array();
    $rdrct['source'] = trim($import[0], ' /');
    $rdrct['redirect'] = trim($import[1], ' /');
    if (!in_array(strtolower($rdrct['source']), $redirects)) {
      $redirects[$rdrct['source']] = strtolower($rdrct['source']);
      if ($form_state['values']['convert_to_system']) {
        $rdrct['redirect'] = path_redirect_tools_convert_to_system($rdrct['redirect']);
        $path_parts = explode('#', $rdrct['redirect']);
        $rdrct['redirect'] = $path_parts[0];
        $rdrct['fragment'] = $path_parts[1];
      }

      // set the rid for update
      if ($import['rid'] !== FALSE && $import['rid'] > 0) {
        $rdrct['rid'] = $import['rid'];
        $updated++;
      }
      else {
        $added++;
      }

      if (trim($import['redirect_code']) != '') {
        $rdrct['type'] = $import['redirect_code'];
      }

      if ($rdrct['source'] != $rdrct['redirect']) {
        path_redirect_save($rdrct);
      }
    }
    else {
      drupal_set_message('Duplicate URL found: ' . check_plain($rdrct['source']));
    }
  }
  return array('added' => $added, 'updated' => $updated);
}

function path_redirect_tools_convert_to_system($url, $action = 'source') {
  $url = trim($url, ' /');
  $paths = explode('#', $url);
  $path = drupal_lookup_path($action, $paths[0]);
  if ($path && isset($paths[1])) {
    $path .= '#' . $paths[1];
  }
  return $path ? $path : $url;
}

/**
 * Check if a url alias exists (by from).
 */
function path_redirect_tools_alias_exists($from) {
  $return = db_result(db_query('SELECT rid FROM {path_redirect} WHERE source = "%s"', $from));
  return $return;
}

/**
 * Do the actual file processing.
 */
function path_redirect_tools_process_upload($file, $process = TRUE) {
  
  if (($handle = fopen($file, "r")) !== FALSE) {
    $row = 0;
    $from = 0;
    $to = 1;
    while (($data = fgetcsv($handle, 1000, ',', '"')) !== FALSE) {
      if ($row == 0) {
        if ($data[$from] != 'old_url' || $data[$to] != 'new_url') {
          form_set_error('source', t('Please make sure column headings match the instructions.'));
          return FALSE;
        }
      }
      else {
        $rid = path_redirect_tools_alias_exists($data[$from]);
        // prep for import
        if ($process) {
          $output[] = array($data[$from], $data[$to], 'rid' => $rid);
        }
        // prep for display
        else {
          $op = intval($rid) > 0 ? 'update' : ($row === 0 ? 'operation' : 'add');
          $to_url = path_redirect_tools_convert_to_system($data[$to]);
          $output[] = array(check_plain($data[$from]), check_plain($to_url), $op);
        }
      }
      $row++;
    }
    fclose($handle);
  }
  
  return $output;
}

/**
 * Export callback.
 */
function path_redirect_tools_export() {
  $filename = 'path_redirect_export.csv';
  drupal_set_header('Content-Type: text/csv');
  drupal_set_header('Content-Disposition: attachment; filename=' . $filename);

  $sql = "SELECT * FROM {path_redirect}";

  $result = db_query($sql);

  $count = mysql_num_fields($result);
  
  for($i = 0; $i < $count; $i++){
    $header[] = mysql_field_name($result, $i);
  }
  print implode(',', $header) ."\r\n";

  while($row = db_fetch_array($result)){
    foreach($row as $value){
      $values[] = '"' . str_replace('"', '""', decode_entities(strip_tags($value))) . '"';
    }
    print implode(',', $values) ."\r\n";
    unset($values);
  }
}