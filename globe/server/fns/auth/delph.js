/* jshint camelcase: false */

'use strict';

var http = require('http');
var groups = require('../../config/groups');


var delph = {

	authenticate: function(data, callback) {
	
		var data = "token=" + data.token;
		var options = {
			host: '127.0.0.1',
			port: '9000',
			method: 'POST',
			headers: {
				'Content-Type': 'application/x-www-form-urlencoded',
				'Content-Length': data.length
			}
		};
		
		var request = http.request(options, function(response){
			response.on('data', function(body){
				
				body = JSON.parse(body);

				if(body.error) {
					return callback('Delphinoid server login token was not accepted: ' + body.error);
				}
				if(body.banned) {
					return callback('Your account on Delphinoid\'s server has been banned.');
				}
	
				var verified = {
					site: 'd',
					name: body.user_name,
					siteUserId: body.user_id,
					avatar: body.avatar,
					group: delph.powerToGroup(body.power),
					beta: body.beta
				};
	
				// remove empty string which messes with validation
				if(!verified.avatar) {
					delete verified.avatar;
				}

				return callback(0, verified);
				
			});
		});
		
		request.write(data);
		request.end();
		
	},


	/**
	 * Convert power (0-3) to group (g,u,m,a)
	 * The gamehub login system assigns a power to users. Higher power = more moderator privileges
	 * Futurism uses groups to do the same thing, so this function turns gamehub powers into futurism groups
	 * @param {number} power
	 * @returns {string}
	 */
	powerToGroup: function(power) {
		var group = groups.GUEST;
		if(power === 1) {
			group = groups.USER;
		}
		if(power === 2) {
			group = groups.MOD;
		}
		if(power === 3) {
			group = groups.ADMIN;
		}
		return group;
	}

};

module.exports = delph;