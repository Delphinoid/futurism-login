# Futurism Login Server

A basic login server for Jiggmin's globe, aimed to be used for Futurism.



## Setup

There are some files that need to be modified first.
```
futurism-client/src/scripts/services/websites/delphinoid.js
globe/server/fns/auth/delph.js
futurism-login/bin/Release/register.html
futurism-login/bin/Release/login.html
```
Starting on lines 19, 14, 5 and 5 respectively, you may modify the host address and port. Now merge the provided globe and futurism-client folders with your own.



## Usage

Build and run futurism-login. Users can register and log in from register.html and login.html. These register session cookies with the server address in their browser, which will be verified when trying to log in.



## Planned

* The server should load a config file that specifies
  * the database location,
  * the host address,
  * session lifetime.
* Passwords should be hashed rather than being stored in plain-text.