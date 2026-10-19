Chicago Burnout — Version 0.2 starter files

Contents:
- source/main.c: Citro3D street scene with road, lane markings, and low-poly buildings
- source/vshader.v.pica: PICA200 vertex shader compiled by the devkitPro Makefile

Before building:
1. Copy main.c into the repository's source/main.c.
2. Add vshader.v.pica into the repository's source/ folder.
3. In Makefile, change `LIBS := -lctru -lm` to:
   `LIBS := -lctru -lcitro3d -lm`
4. Commit the changes and check GitHub Actions.

This is an early 3D environment prototype, not yet a full open-world game. Test the resulting .3dsx on your own console; successful compilation alone does not guarantee hardware behavior.
