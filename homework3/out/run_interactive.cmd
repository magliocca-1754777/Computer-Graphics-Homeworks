.\bin\debug\yscenegen tests\01_terrain\terrain.json -o outs\01_terrain\terrain.json --terrain object
.\bin\debug\yscenegen tests\02_displacement\displacement.json -o outs\02_displacement\displacement.json --displacement object
.\bin\debug\yscenegen tests\03_hair1\hair1.json -o outs\03_hair1\hair1.json --hairbase object --hair hair
.\bin\debug\yscenegen tests\03_hair2\hair2.json -o outs\03_hair2\hair2.json --hairbase object --hair hair --hairlen 0.005 --hairstr 0
.\bin\debug\yscenegen tests\03_hair3\hair3.json -o outs\03_hair3\hair3.json --hairbase object --hair hair --hairlen 0.005 --hairstr 0.01
.\bin\debug\yscenegen tests\03_hair4\hair4.json -o outs\03_hair4\hair4.json --hairbase object --hair hair --hairlen 0.02 --hairstr 0.001 --hairgrav 0.0005 --hairstep 8
.\bin\debug\yscenegen tests\04_grass\grass.json -o outs\04_grass\grass.json --grassbase object --grass grass

.\bin\debug\ysceneitraces outs\01_terrain\terrain.json -o out\01_terrain.jpg -s 256 -r 720
.\bin\debug\ysceneitraces outs\02_displacement\displacement.json -o out\02_displacement.jpg -s 256 -r 720
.\bin\debug\ysceneitraces outs\03_hair1\hair1.json -o out\03_hair1.jpg -s 256 -r 720
.\bin\debug\ysceneitraces outs\03_hair2\hair2.json -o out\03_hair2.jpg -s 256 -r 720
.\bin\debug\ysceneitraces outs\03_hair3\hair3.json -o out\03_hair3.jpg -s 256 -r 720
.\bin\debug\ysceneitraces outs\03_hair4\hair4.json -o out\03_hair4.jpg -s 256 -r 720
.\bin\debug\ysceneitraces outs\04_grass\grass.json -o out\04_grass.jpg -s 256 -r 720 -b 128

.\bin\debug\yscenegen tests\voronoise\voronoise.json -o outs\voronoise\voronoise.json --voronoise object
.\bin\debug\ysceneitraces outs\voronoise\voronoise.json -o out\voronoise.jpg -s 256 -r 720

.\bin\debug\yscenegen tests\smoothvoronoi\smoothvoronoi.json -o outs\smoothvoronoi\smoothvoronoi.json --smoothvoronoi object
.\bin\debug\ysceneitraces outs\smoothvoronoi\smoothvoronoi.json -o out\smoothvoronoi.jpg -s 256 -r 720

.\bin\debug\yscenegen tests\voronoiedges\voronoiedges.json -o outs\voronoiedges\voronoiedges.json --voronoiedges object
.\bin\debug\ysceneitraces outs\voronoiedges\voronoiedges.json -o out\voronoiedges.jpg -s 256 -r 720

.\bin\debug\yscenegen tests\poxo\poxo.json -o outs\poxo\poxo.json --poxo object
.\bin\debug\ysceneitraces outs\poxo\poxo.json -o out\poxo.jpg -s 256 -r 720

.\bin\debug\yscenegen tests\cellnoise\cellnoise.json -o outs\cellnoise\cellnoise.json --cellnoise object
.\bin\debug\ysceneitraces outs\cellnoise\cellnoise.json -o out\cellnoise.jpg -s 256 -r 720