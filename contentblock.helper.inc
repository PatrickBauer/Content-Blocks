<?php

/**
 * Helper class needed at serveral locations
 * Defines ContentBlockHelper as a singleton class, used for repetetive tasks
 */

class ContentBlockHelper {
  /**
   * Singleton instance
   */
  private static $instance = NULL;
  
  /**
   * Array of all available blocktypes
   */
  private $block_types = NULL;
  
  /**
   * Returns the singleton instance
   */
  public static function getInstance() {
    if(self::$instance == NULL){
      self::$instance = new ContentBlockHelper();
    }
    
    return self::$instance;
  } 
  
  /**
   * Returns a list of all available blocktypes
   */
  public function getBlockTypes($rebuild = false) {
    if($rebuild || $this->block_types == NULL) {
      $this->block_types = $this->buildBlockTypes();
    }
    
    return $this->block_types;
  }
  
  /**
   * Builds an array containing all available blocktypes
   */
  private function buildBlockTypes() {
     $types = array();
  
    //select all types from database
    $query = db_select('contentblock_type', 'cbt')
      ->fields('cbt');

    //fill array
    foreach ($query->execute() as $type) {
      $types[$type->type] = $type;
    }

    return $types;
  }
}
