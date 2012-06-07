<?php

class ContentBlockHelper {
  /**
   * Singleton instance
   */
  private static $instance = NULL;
  
  /**
   * Array of all available contentblock types
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
   * Returns a list of all available contentblock types
   */
  public function getBlockTypes() {
    if($this->block_types == NULL) {
      $this->block_types = $this->buildBlockTypes();
    }
    
    return $this->block_types;
  }
  
  
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
