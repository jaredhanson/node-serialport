{
  'targets': [
    {
      'target_name': 'serialport_native',
      'type': 'shared_library',
      'product_extension': 'node',
      'sources': [ 'serialport_native/serialport_native.cc' ],
      'include_dirs': [
	      '<(NODE_ROOT)/src',
		  '<(NODE_ROOT)/deps/v8/include',
		  '<(NODE_ROOT)/deps/uv/include',
	    ],
	  'conditions': [
	      [ 'OS=="win"', {
	        'libraries': [ '-l<(NODE_ROOT)/<(node_lib_folder)/node.lib'  ],
	        'msvs-settings': {
	          'VCLinkerTool': {
	            'SubSystem': 3, # /subsystem:dll
	          },
	        }
	      } ]
	    ]
    }
  ]
}