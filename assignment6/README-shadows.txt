The starter code handles most of the annoying opengl cruft that needs to be set up for
shadow mapping, so that you get to code just the meaningful stuff. It's still
not yet at all trivial; most of the mechanisms have to be understood first, and
you might need to touch some hairy parts to do more cool stuff. Some debug
features are there too. Fill in your code to "YOUR SHADOWS HERE". Both C++ and
GLSL need a bit of hacking. Note that you need to study on the topic yourself -
that's the point of the extras.

You will need to configure not only the light positions but also their
directions (or strictly, orientations), and possibly FOVs.

The sample solution includes a rotating white light, another rotating red light
and a stationary green light. Note that e.g. the green light does not affect
shoulders, the whole head and especially its nose cast some big shadows when
the rotating lights are in proper positions, and if you look carefully, there
are black areas behind the ears when applicable, too. Try playing with the
"shadow epsilon" slider; it affects on how the shade test comparison is done.

Protip: render also a floor below the head, and write shaders for it that read
the shadow maps, so that the head casts shadows on the floor.
