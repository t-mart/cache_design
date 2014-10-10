module.exports = function(grunt) {

  // Project configuration.
  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json'),
    shell: {
      compile: {
        command: 'make'
      },
      clean: {
        command: 'make clean'
      },
      run: {
        command: './cachesim -v 0 -k 0 < traces/perlbench.trace'
      }
    },
    watch: {
      code: {
        files: ['*.c', '*.h'],
        tasks: ['run'],
      },
    },
  });

  grunt.loadNpmTasks('grunt-contrib-watch');
  grunt.loadNpmTasks('grunt-shell');

  grunt.registerTask(
    'compile',
    'compile c',
    ['shell:compile']
  );

  grunt.registerTask(
    'run',
    'run the code',
    ['shell:clean','compile', 'shell:run']
  );

  grunt.registerTask('default', ['watch:code']);

};
