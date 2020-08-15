# one_time_pads
#### Assignment 3 - David Vega 8/2/20

```
In this assignment, you will be creating five small programs that encrypt 
and decrypt information using a one-time pad-like system. These programs will 
combine the multi-processing code you have been learning with socket-based 
inter-process communication. Your programs will also be accessible from the 
command line using standard Unix features like input/output redirection, and
job control. Finally, you will write a short compilation script.
```

Hello TA. Thank you for reviewing my program. 

To run the program please do the following. 

1. unzip vegada_assignment3.zip
2. chmod +x compileall p3gradingscript
3. compileall
4. p3gradingscript port1 port2 > mytestresults 2>&1

special notes:

> My code compiles with -std=c99. It's included in my compile file. But just
> in case you use your own, please make sure to include -std=c99. 

> please keep in mind that my code uses `setsockopt` which gives users the
> ability to reuse ports.  For running multiple tests, don't worry about
> changing the ports. 

> Please make sure the directory you run my code in, is included as a path in
> .bashrc, in order to get the p3gradingscript to work. To use ./ you would
> have to re-do the grading script since none of the commands include it. 

*I know you don't need any of this info, that's why your a TA but I'm
 including it just to be safe*
 
 #####Thank you!