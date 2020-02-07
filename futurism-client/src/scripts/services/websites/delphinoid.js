angular.module('futurism')
    .factory('delphinoid', function($, memory) {
        'use strict';
        
        var self = {
            
            id: 'd',
            name: 'Delphinoid',
            
            tryLogin: function(callback) {
                return self.checkLogin(callback);
            },
            
            checkLogin: function(callback) {
                if(memory.long.get('site') !== 'd') {
                    return callback('Delphinoid\'s server is not the selected site');
                }
                
                $.ajax('http://127.0.0.1:9000', {
                    type: 'GET',
                    dataType: 'jsonp',
                    xhrFields: {
                        withCredentials: true
                    }
                })
                .done(function(data) {
                    if(data.logged_in) {
                        data.site = 'd';
                        return callback(null, data);
                    }
                    else {
                        return callback('not logged into Delphinoid\'s server');
                    }
                })
                .error(callback);
            },
            
            logout: function() {
                
            }
        };
        
        return self;
    });