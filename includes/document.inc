<?php

/**
 * @file
 * Menu callbacks and hooks that are related to fetching / saving documents.
 */

/**
 * Menu callback that returns a JSON array which lists all document identifiers.
 *
 * We don't implement it in since we don't want the user to be able to load
 * every possible document from the same interface.
 */
function islandora_cwrc_writer_documents() {
  // We don't utilize this functionality currently. And as such
  // 'editor/documents/%' is not implemented.
  drupal_json_output(array());
  drupal_exit();
}

/**
 * Endpoint for GET, PUT, DELETE requests from the CWRC-Writer for documents.
 *
 * @todo For now assume the datastream is always CWRC, but we should have a hook
 * that checks the Content Model of the object then hooks other modules to say
 * what datastream should be modified. Or we could make the editor aware of what
 * datastream is being edited, it could be used as the document id, like
 * #pid/dsid.
 *
 * @param string $pid
 *   The PID of the object to fetch / modify.
 * @param string $dsid
 *   The DSID containing the document, right now assumed to be CWRC.
 */
function islandora_cwrc_writer_document($pid, $dsid = 'CWRC') {
  module_load_include('inc', 'islandora_rest', 'includes/utilities');
  module_load_include('inc', 'islandora_rest', 'includes/datastream');
  module_load_include('inc', 'islandora_cwrc_writer', 'includes/utilities');
  $request_method = islandora_rest_get_request_method();
  try {
    // Fetch the requested resource. Throws 404, if the resource is not found.
    $resource = islandora_rest_get_requested_resource($pid, $dsid);
    // Check Drupal Permissions. Throws 401, 403, if access is denied.
    islandora_rest_require_access('datastream', $request_method, $resource);
    // Generate the Response, we don't implement POST / DELETE as we don't
    // currently allow the user to make new documents or remove them.
    switch ($request_method) {
      case 'GET':
        $parameters = array(
          'resource' => $resource,
          'request' => array(
            'version' => FALSE,
            'content' => TRUE,
          ),
        );
        $response = islandora_rest_datastream_get_response($parameters);
        break;

      case 'PUT':
        // The default islandora rest module PUT response won't work in this
        // case as CWRC-Writer does not do it as a POST (Means it won't work in
        // IE), but I don't think they care about that. So we have to implement
        // actual PUT requests now. I'm not sure if this function is ready for
        // show-time, perhaps we'll integrated it with Islandora Rest at a later
        // time.
        $parameters = array(
          'resource' => $resource,
          'request' => array(
            'version' => FALSE,
            'content' => TRUE,
          ),
        );
        $response = islandora_cwrc_writer_update_document($parameters['resource']);
        break;

      default:
        throw new Exception('Not Implemented', 500);
    }
  }
  catch (Exception $e) {
    drupal_add_http_header('Status', $e->getCode());
    // CWRC-Writer currently ignores the response aside from the status code,
    // when an error occurs.
    $response = array('message' => $e->getMessage());
  }
  $encode = is_array($response) || is_object($response);
  if ($encode) {
    // Responses that don't generate objects or arrays, must set their own
    // content types. Not calling drupal_json_output because we are setting
    // encoding.
    drupal_add_http_header('Content-Type', 'application/json; utf-8');
    drupal_json_output($response);
  }
  else {
    echo $response;
  }
  drupal_exit();
}

/**
 * Responds to the POST request to update the documents schema.
 *
 * Modifies and existing datastream, with the uploaded content, also adds a
 * processing instruction for the chosen schema.
 *
 * @param string $pid
 *   The PID of the object to fetch / modify.
 * @param string $dsid
 *   The DSID containing the document, right now assumed to be CWRC.
 */
function islandora_cwrc_writer_document_schema($pid, $dsid = 'CWRC') {
  module_load_include('inc', 'islandora_cwrc_writer', 'includes/utilities');
  drupal_json_output(array('success' => TRUE));
}

/**
 * Responds to the PUT request to update the document datastream.
 *
 * Modifies and existing datastream, with the uploaded content, also adds a
 * processing instruction for the chosen schema.
 *
 * @param AbstractDatastream $datastream
 *   The datastream to update with the put contents.
 *
 * @return array
 *   JSON indicating that the PUT was successful.
 */
function islandora_cwrc_writer_update_document(AbstractDatastream $datastream) {
  module_load_include('inc', 'islandora_cwrc_writer', 'includes/utilities');
  $temp_name = drupal_tempnam('public://', 'file');
  $input = fopen('php://input', 'r');
  $temp = fopen($temp_name, 'w');
  while ($data = fread($input, 1024)) {
    fwrite($temp, $data);
  }
  fclose($input);
  fclose($temp);
  $datastream->setContentFromFile($temp_name);
  drupal_unlink($temp_name);
  return array('success' => TRUE);
}