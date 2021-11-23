# ./bin/ymodel --scene tests/01_terrain/terrain.json --output outs/01_terrain/terrain.json --terrain object
./bin/ymodel --scene tests/02_displacement/displacement.json --output outs/02_displacement/displacement.json --displacement object
# ./bin/ymodel --scene tests/03_hair1/hair1.json --output outs/03_hair1/hair1.json --hairbase object --hair hair --dense_hair
# ./bin/ymodel --scene tests/03_hair1_density/hair1.json --output outs/03_hair1/hair1_dense.json --hairbase object --hair hair --dense_hair --hairlen 0.001 --hairstr 0.00000000001 --hairstep 8
# ./bin/ymodel --scene tests/03_hair2/hair2.json --output outs/03_hair2/hair2.json --hairbase object --hair hair --hairlen 0.005 --hairstr 0
# ./bin/ymodel --scene tests/03_hair3/hair3.json --output outs/03_hair3/hair3.json --hairbase object --hair hair --hairlen 0.005 --hairstr 0.01
# ./bin/ymodel --scene tests/03_hair4/hair4.json --output outs/03_hair4/hair4.json --hairbase object --hair hair --hairlen 0.02 --hairstr 0.001 --hairgrav 0.0005 --hairstep 8
# ./bin/ymodel --scene tests/04_grass/grass.json --output outs/04_grass/grass.json --grassbase object --grass grass

# ./bin/yscene render outs/01_terrain/terrain.json --output out/01__voro_terrain.jpg --samples 25 --resolution 720
./bin/yscene render outs/02_displacement/displacement.json --output out/02_voro_displacement.jpg --samples 256 --resolution 720
# ./bin/yscene render outs/03_hair1/hair1.json --output out/03_hair1.jpg --samples 5 --resolution 720
# ./bin/yscene render outs/03_hair1/hair1_dense.json --output out/03_hair1_density.jpg --samples 5 --resolution 720
# ./bin/yscene render outs/03_hair2/hair2.json --output out/03_hair2.jpg --samples 60 --resolution 720
# ./bin/yscene render outs/03_hair3/hair3.json --output out/03_hair3.jpg --samples 60 --resolution 720
# ./bin/yscene render outs/03_hair4/hair4.json --output out/03_hair4.jpg --samples 60 --resolution 720
# ./bin/yscene render outs/04_grass/grass.json --output out/04_grass.jpg --samples 25 --resolution 720 --bounces 128
