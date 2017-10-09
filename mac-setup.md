Setting up Docker
------------------
docker-machine start default 
eval $(docker-machine env)

Dependencies
------------------

brew install gcc (4.8.1)

.bashrc
```
export CXX=/usr/local/bin/c++
export CXXCPP='/usr/local/bin/c++ -E'
```

(Need to build these dependencies locally with the gcc compiler)
- boost 1.65.1 http://www.boost.org/users/history/version_1_65_1.html
- geos 3.6.2 https://trac.osgeo.org/geos
- protobuf c https://github.com/protobuf-c/protobuf-c

brew link sqlite3 --force
echo 'export PATH="/usr/local/opt/sqlite/bin:$PATH"' >> ~/.bash_profile

Building
------------------
```
git submodule update --init --recursive
./autogen.sh

./configure \
--with-boost-libdir=/Users/mcheung/git/boost_1_65_1/stage/lib (you can also put boost into your LD_LIBRARY_PATH)

make test -j$(nproc)

sudo make install
```

Setting up transit.land datastorage
----------------------------
```
git clone git@github.com:transitland/transitland-datastore.git
docker up

docker ps 
SSH into the docker container - docker exec -it <container id> /bin/bash

bundle exec rake enqueue_feed_fetcher_workers
bundle exec rake enqueue_feed_eater_worker[f-9q9-bart,'',6]
bundle exec rake enqueue_feed_eater_worker[f-9q9-caltrain,'',6]

bundle exec sidekiq
```

Generate tiles and data
-------------------------------
```
wget https://s3.amazonaws.com/metro-extracts.mapzen.com/san-francisco-bay_california.osm.pbf
mkdir -p valhalla_tiles

valhalla_build_config --mjolnir-tile-dir ${PWD}/valhalla_tiles --mjolnir-tile-extract ${PWD}/valhalla_tiles.tar --mjolnir-timezone ${PWD}/valhalla_tiles/timezones.sqlite --mjolnir-admin ${PWD}/valhalla_tiles/admins.sqlite > valhalla.json

valhalla_build_transit valhalla.json http://transitlanddatastore_app_1:3000 10000

valhalla_build_tiles -c valhalla.json san-francisco-bay_california.osm.pbf

find valhalla_tiles | sort -n | tar cf valhalla_tiles.tar --no-recursion -T -
```

Starting Service
-----------------
`valhalla_route_service valhalla.json 1`

Command line
-----------------
```
valhalla_run_route -j '{"locations":[{"lat":37.777153,"lon":-122.392161,"type":"break","city":"San Francisco","state":"CA"},{"lat":37.391357,"lon":-122.080358,"type":"break","city":"Mountain view","state":"CA"}],"costing":"auto","directions_options":{"units":"miles"}}' --config valhalla.json
```


