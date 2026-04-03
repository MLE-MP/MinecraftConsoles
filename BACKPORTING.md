# Approach to Backported Features
All backported features incorperated into MinecraftConsoles should be, when merged, functionally identical to their state in the version of the game we're currently targeting. This should be in reference to a known 4J build of LCE. Verification can either be done by doing a decompilation based match of the implementation or, alternatively, all functionality and limitations of the given feature should be compared against the version of LCE we're targeting.

# Approach to Bugfixes
Anything that does not behave in an "expected" manner, especially if its behavior is not widely accepted as a gameplay mechanic, is valid for fixing in our repository. This includes bugfixes that were made in versions past the version we target, but excludes any visual changes that may not have been included at the build we're targeting.

If you provide a visual bugfix that fixes a distinctive quirk of the LCE renderer, it should be provided in an "off by default" state that can be toggled on in-game by the user. There is no guarantee that we will merge it.

If your visual bugfix is a fix added in a future version of LCE than the one we're targeting, it should also be put behind a toggle or equivalent system for keeping it off by default.

# Targeted Version
We are currently accepting backports for up to and including TU24. Feature backports from TU25 and above will not be accepted.

# Original Codebase
MinecraftConsoles is based on a WIP build of TU19, built on top of the December 2014 codebase. 
