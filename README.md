# shlab
Final Project for CMPSC311: Intro to Systems Programming

## Code written by me:
  - proxy.c
  
## Code given by instructor:
  - Everything Else
  
Simple HTTP 1.0 proxy server that works on tiny test server, as well as espn.com, bbc.com, web.mit.edu, and any other HTTP 1.0 website. Read uploaded README for more info


### Commented code at line 150
I was going to implement a cache using a doubly linked list, and use mmap to move the payload into the cache memory and memcpy to adjust its order in the cache based on access count.
