# ./bin/ymodel --scene tests/01_terrain/terrain.json --output outs/01_terrain/terrain.json --terrain object
# ./bin/ymodel --scene tests/02_displacement/displacement.json --output outs/02_displacement/displacement.json --displacement object --voronoise_u 0 --voronoise_v 0
# ./bin/ymodel --scene tests/03_hair1/hair1.json --output outs/03_hair1/hair1.json --hairbase object --hair hair --dense_hair
# ./bin/ymodel --scene tests/03_hair1_density/hair1.json --output outs/03_hair1/hair1_dense.json --hairbase object --hair hair --dense_hair --hairlen 0.001 --hairstr 0.00000000001 --hairstep 8
# ./bin/ymodel --scene tests/03_hair2/hair2.json --output outs/03_hair2/hair2.json --hairbase object --hair hair --hairlen 0.005 --hairstr 0
# ./bin/ymodel --scene tests/03_hair3/hair3.json --output outs/03_hair3/hair3.json --hairbase object --hair hair --hairlen 0.005 --hairstr 0.01
# ./bin/ymodel --scene tests/03_hair4/hair4.json --output outs/03_hair4/hair4.json --hairbase object --hair hair --hairlen 0.02 --hairstr 0.001 --hairgrav 0.0005 --hairstep 8
# ./bin/ymodel --scene tests/04_grass/grass.json --output outs/04_grass/grass.json --grassbase object --grass grass

# ./bin/yscene render outs/01_terrain/terrain.json --output out/01__voro_terrain.jpg --samples 25 --resolution 720
# ./bin/yscene render outs/02_displacement/displacement.json --output out/02_voro_displacement_0_0.jpg --samples 25 --resolution 720
# ./bin/yscene render outs/03_hair1/hair1.json --output out/03_hair1.jpg --samples 5 --resolution 720
# ./bin/yscene render outs/03_hair1/hair1_dense.json --output out/03_hair1_density.jpg --samples 5 --resolution 720
# ./bin/yscene render outs/03_hair3/hair3.json --output out/03_hair3.jpg --samples 60 --resolution 720
# ./bin/yscene render outs/03_hair4/hair4.json --output out/03_hair4.jpg --samples 60 --resolution 720
# ./bin/yscene render outs/04_grass/grass.json --output out/04_grass.jpg --samples 25 --resolution 720 --bounces 128

# ./bin/ymodel --scene tests/03_sample_elimination/sample_elimination.json --output outs/03_hair2/hair2.json --hairbase object --hair hair --hairlen 0.001 --hairstr 0.00000000001 --hairnum 2000
# ./bin/yscene render outs/03_hair2/hair2.json --output out/03_hair2_normal_2000.jpg --samples 60 --resolution 720

# ./bin/ymodel --scene tests/03_sample_elimination/sample_elimination.json --output outs/03_hair2/hair2.json --hairbase object --hair hair --hairlen 0.001 --hairstr 0.00000000001 --hairnum 200 --sample_elimination --influence_radius 0.01 --cell_size 0.05
# ./bin/yscene render outs/03_hair2/hair2.json --output out/03_hair2_sample_elimination_200-0_01r-0_05c.jpg --samples 60 --resolution 720

# for i in 100 130 160 200 230 260 300; do
#     ./bin/ymodel --scene tests/05_tree/tree.json --output outs/05_tree/tree.json --tree --brsteps $i
#     ./bin/yscene render outs/05_tree/tree.json --output out/05_tree.jpg --samples 2 --resolution 720
# done

./bin/ymodel --scene tests/05_tree/tree.json --output outs/05_tree/tree.json --tree --brsteps 500
./bin/yscene render outs/05_tree/tree.json --output out/05_tree.jpg --samples 2 --resolution 720

./bin/ymodel --scene tests/05_tree/tree.json --output outs/05_tree/tree.json --tree --brsteps 1000
./bin/yscene render outs/05_tree/tree.json --output out/05_tree.jpg --samples 2 --resolution 720

./bin/ymodel --scene tests/05_tree/tree.json --output outs/05_tree/tree.json --tree --brsteps 5000
./bin/yscene render outs/05_tree/tree.json --output out/05_tree.jpg --samples 2 --resolution 720

./bin/ymodel --scene tests/05_tree/tree.json --output outs/05_tree/tree.json --tree --brsteps 10000
./bin/yscene render outs/05_tree/tree.json --output out/05_tree.jpg --samples 2 --resolution 720

./bin/ymodel --scene tests/05_tree/tree.json --output outs/05_tree/tree.json --tree --brsteps 30000
./bin/yscene render outs/05_tree/tree.json --output out/05_tree.jpg --samples 2 --resolution 720

./bin/ymodel --scene tests/05_tree/tree.json --output outs/05_tree/tree.json --tree --brsteps 50000
./bin/yscene render outs/05_tree/tree.json --output out/05_tree.jpg --samples 2 --resolution 720
