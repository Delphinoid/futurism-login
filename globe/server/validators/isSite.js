'use strict';

var sites = require('../config/sites');

var isSite = function(val) {
	return [sites.GUEST, sites.JIGGMIN, sites.KONGREGATE, sites.NEWGROUNDS, sites.FACEBOOK, sites.ARMOR_GAMES, sites.DELPHINOID].indexOf(val) !== -1;
};

module.exports = isSite;