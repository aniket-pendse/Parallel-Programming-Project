#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include "com/here/pb/hdmap/external/v1/lanes/layer-lane-geometry-polyline.pb.h"

using namespace std;

using namespace com::here::pb::hdmap::external::v1::geometry;
using namespace com::here::pb::hdmap::external::v1::lanes;

ofstream ofs;
std::string filename = "laneData.csv";

int twos_complement_to_decimal(string input)
{
	
	int n = input.length();
	//cout<<"Length "<<n<<endl;
	char a = input[0];
	//cout<<"MSB "<<stol(&a,nullptr,2)<<endl;
	long int sum = -stol(&a,nullptr,2)*pow(2,n-1);
	for(int i = 1; i < n; i++)
	{
	//cout<<"Sum "<<sum<<endl;
	a = input[i];
	sum += stol(&a,nullptr,2)*pow(2, n-1-i);
	}
	//cout<<"Sum value "<<sum<<endl;
	//sum = sum - stoi(&input[n-1],nullptr,2)*pow(2,n);
	return sum;
}
std::pair<double, double> decode_morton_2d(int64_t m)
{
    //Converting number to 64 bits
    string buffer;
    string odd, even;
    int i=0;
    while (i<64)
    {
        buffer.push_back('0' + (m & 1));
	m>>=1;
	i++;
    } 
    std::reverse(buffer.begin(), buffer.end());
    //cout<<"First bit "<<buffer<<endl;
    for(int i = 1; i<64;i++)
    {
	if(i%2 == 0)
	    even.push_back(buffer[i]);//latitude
	else
	    odd.push_back(buffer[i]);//longitude
    }
    //odd.push_back('\0');
    //even.push_back('\0');
    //cout<<"odd before is "<<odd<<endl;
    even = even[0] + even;
    //cout<<"Length "<<even.length()<<endl;
    //cout<<"Even after  is "<<even<<endl;
    long int lon = twos_complement_to_decimal(odd);
    long int lat = twos_complement_to_decimal(even);
   
    //cout<<"Lat is "<<lat<<endl;
 		   //00000000000000101001110101111111
    //string test = "00000000000000110010001011100001";
    //long int test_val = twos_complement_to_decimal(test);
    //cout<<"Test value "<<test_val/pow(2,31)*180<<endl;
    //long int x = (lon );//- 1);
    //long int y = (lat );//- 1);
    //cout<<"New lat value "<<lat/pow(2,31)*180<<endl;

    double x_new = lon/pow(2,31)*180;
    double y_new = lat/pow(2,31)*180;
   
    
  return std::make_pair(x_new,y_new);
}

void ListLanes(const LaneGeometryPolylineLayerTile& tile) {
  ofs.open(filename);
  int lane_id = 0;
  int64_t current_2d_encoding_int = tile.tile_center_here_3d_coordinate().here_2d_coordinate();
  int64_t temp = current_2d_encoding_int;
cout<<"Tile center : "<<current_2d_encoding_int<<endl;
  std::pair<double, double> decoded_tile_center = decode_morton_2d(current_2d_encoding_int);
  cout<<"Long "<<decoded_tile_center.first<< " Lat "<<decoded_tile_center.second<<endl;
  //cout<<"Tile center : "<<current_2d_encoding_int<<endl;
  //cout<<"Tile Id: "<<tile.here_tile_id()<<endl;
  for (int i = 0; i < tile.lane_group_geometries_size(); i++) {
    LaneGroupGeometry groupGeometry = tile.lane_group_geometries()[i];
    for (int j = 0; j < groupGeometry.lane_geometries_size(); j++) {
      double lat = decoded_tile_center.second; double lon = decoded_tile_center.first;
      current_2d_encoding_int = 0;
      LineString3dOffset theLanePathGeometry = groupGeometry.lane_geometries()[j].lane_path_geometry();
      for (int k = 0; k < theLanePathGeometry.here_2d_coordinate_diffs().size(); k++) {
        
	int64_t here_2d_coordinate_diff = theLanePathGeometry.here_2d_coordinate_diffs()[k];
	//decode_morton_2d(here_2d_coordinate_diff);
	int64_t theXor = (here_2d_coordinate_diff ^ current_2d_encoding_int);
	std::pair<double, double> decoded = decode_morton_2d(theXor);
	/// Saving for next iteration
	current_2d_encoding_int = theXor;
	lat = decoded.second;
	lon = decoded.first;

	if(lon>(0.02197265625/2))
		lon = -(0.02197265625) +lon;
	if(lat>(0.02197265625/2))
		lat = -(0.02197265625) +lat;
	
	//cout<<"k "<<k<<endl;
	//cout<<"lat: "<<lat<<" long: "<<lon<<endl;
	ofs << lat << "," << lon<<","<<lane_id<<std::endl;
	
      }
      lane_id++;
    }


  }
  ofs.close();
}
//0.0784735/2
int main(int argc, char* argv[]) {
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  if (argc != 2) {
    cerr << "Usage:  " << argv[0] << " LANDMARK_FILE" << endl;
    return -1;
  }

  LaneGeometryPolylineLayerTile tile;

  {
    // Read the existing address book.
    fstream input(argv[1], ios::in | ios::binary);
    if (!tile.ParseFromIstream(&input)) {
      cerr << "Failed to parse address book." << endl;
      return -1;
    }
  }

  ListLanes(tile);

  // Optional:  Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();

  return 0;
}



