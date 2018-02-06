/*
 
ESP8266 file system builder with PlatformIO support
 
Copyright (C) 2016 by Xose Pérez &lt;xose dot perez at gmail dot com&gt;
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program.  If not, see &lt;http://www.gnu.org/licenses/&gt;.
 
*/
 
// -----------------------------------------------------------------------------
// File system builder
// -----------------------------------------------------------------------------
 
const gulp = require('gulp');
const plumber = require('gulp-plumber');
const htmlmin = require('gulp-htmlmin');
const cleancss = require('gulp-clean-css');
const uglify = require('gulp-uglify');
const gzip = require('gulp-gzip');
const del = require('del');
const useref = require('gulp-useref');
const gulpif = require('gulp-if');
const inline = require('gulp-inline');
const argv = require('yargs').argv;
 
/* Clean destination folder */
gulp.task('clean', function() {
    return del(['data/*']);
});
 
/* Copy static files */
gulp.task('files', function() {
    return gulp.src([
            'data.unminified/**/*.{jpg,jpeg,png,ico,gif}',
            'data.unminified/fsversion'
        ])
        .pipe(gulp.dest('data'));
});

var src = argv.src ? argv.src.split(',') : 'data.unminified/*.htm';

/* Process HTML, CSS, JS  --- INLINE --- */
gulp.task('inline', function() {
    return gulp.src(src)
        .pipe(inline({
            base: 'data.unminified/',
            css: cleancss,
            disabledTypes: ['svg', 'img']
            }))
        .pipe(htmlmin({
            collapseWhitespace: true,
            removeComments: false,
	        minifyJS: { mangle: false }
            }))	        
	    .pipe(gzip())
        .pipe(gulp.dest('data'));
})
 
/* Process HTML, CSS, JS */
gulp.task('html', function() {
    return gulp.src('data.unminified/*.htm')
        .pipe(useref())
        .pipe(plumber())  
	.pipe(gulpif('*.css', cleancss()))
        .pipe(gulpif('*.js', uglify()))      
        .pipe(gulpif('*.htm', htmlmin({
            collapseWhitespace: true,
            removeComments: true,
            minifyCSS: true,
            minifyJS: true
        })))
	.pipe(inline({
	    base: 'data.unminified/',
	    js: uglify,
	    css: cleancss,
        disabledTypes: ['svg', 'img']
	 }))
        .pipe(gzip())
        .pipe(gulp.dest('data'));
});
 
/* Build file system */
gulp.task('buildfs', ['clean', 'files', 'inline']);
gulp.task('default', ['buildfs']);
 
// -----------------------------------------------------------------------------
// PlatformIO support
// -----------------------------------------------------------------------------
 
const spawn = require('child_process').spawn;
 
var platformio = function(target) {
    var args = ['run'];
    if ("e" in argv) { args.push('-e'); args.push(argv.e); }
    if ("p" in argv) { args.push('--upload-port'); args.push(argv.p); }
    if (target) { args.push('-t'); args.push(target); }
    const cmd = spawn('platformio', args);
    cmd.stdout.on('data', function(data) { console.log(data.toString().trim()); });
    cmd.stderr.on('data', function(data) { console.log(data.toString().trim()); });
}
 
gulp.task('uploadfs', ['buildfs'], function() { platformio('uploadfs'); });
gulp.task('upload', function() { platformio('upload'); });
gulp.task('run', function() { platformio(false); });
