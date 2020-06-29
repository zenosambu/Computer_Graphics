# CS-C3100 Computer Graphics, Fall 2019
# Lehtinen / Aarnio, Kemppinen, Rehn
#
# Assignment 2: Curves and Surfaces

Student name: Zeno Sambugaro
Student number:785367
Hours spent on requirements (approx.): 10
Hours spent on extra credit (approx.): 15

# First, some questions about how you worked on this assignment.
# Your answers in this section will be used to improve the course.
# They will not be judged or affect your points, but please answer them.

- Did you go to lectures? Often

- Did you go to exercise sessions? No

- Did you work on the assignment using Aalto computers, your own computers, or both? My own computer

# Which parts of the assignment did you complete? Mark them 'done'.
# You can also mark non-completed parts as 'attempted' if you spent a fair amount of
# effort on them. If you do, explain the work you did in the problems/bugs section
# and leave your 'attempt' code in place (commented out if necessary) so we can see it.

                        R1 Evaluating Bezier curves (2p): done
                      R2 Evaluating B-spline curves (2p): done
       R3 Subdividing a mesh into smaller triangles (2p): done
        R4 Computing positions for the new vertices (2p): done
R5 Smoothing the mesh by repositioning old vertices (2p): done

# Did you do any extra credit work? Yes
(Describe what you did and, if there was a substantial amount of work involved, how you did it. Also describe how to use/activate your extra features, if they are interactive.)
-Proper handling of boundaries for Loop subdivision
-Surfaces of revolution
-Generalized cylinders
-Implement another type of spline curve, and extend the SWP format and parser to accommodate it:
I implemented the Catmull-Rom spline. In order to create a new text file readable by the parse you have to write at the beginning of the first line "cat3" or "cat2" if it is
in 2 dimensions. There is an example file inside "swp", named "skere".
In order to complete the extra credit points I spent about 15 hours.



# Are there any known problems/bugs remaining in your code? No

(Please provide a list of the problems. If possible, describe what you think the cause is, how you have attempted to diagnose or fix the problem, and how you would attempt to diagnose or fix it if you had more time or motivation. This is important: we are more likely to assign partial credit if you help us understand what's going on.)

# Did you collaborate with anyone in the class?
(Did you help others? Did others help you? Let us know who you talked to, and what sort of help you gave or received.)
In completing the assignment I talked with Giuseppe Spallitta and Francesco De Sio.

# Any other comments you'd like to share about the assignment or the course so far? 
(Was the assignment too long? Too hard? Fun or boring? Did you learn something, or was it a total waste of time? Can we do something differently to help you learn? Please be brutally honest; we won't take it personally.)

