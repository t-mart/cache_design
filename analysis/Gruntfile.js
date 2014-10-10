module.exports = function(grunt) {

  // Project configuration.
  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json'),
    watch: {
      site: {
        files: ['*.html', '*.js', '*.css'],
        options: {
          livereload: true
        }
      }
    },
    connect: {
      server: {
        options: {
          livereload: true,
          keepalive: true
        }
      }
    },
    concurrent: {
      options: {
        logConcurrentOutput: true
      },
      watchAndServe: {
        tasks: ['watch:site', 'connect:server']
      }
    }
  });

  grunt.loadNpmTasks('grunt-contrib-watch');
  grunt.loadNpmTasks('grunt-contrib-connect');
  grunt.loadNpmTasks('grunt-concurrent');

  // Default task(s).
  grunt.registerTask('default', ['concurrent:watchAndServe']);

};
