#include "ContigLength.h"
#include "ContigNode.h" // for g_contigIDs
#include <cerrno>
#include <cstdlib>
#include <cstring> // for strerror
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace std;

namespace opt {
	extern unsigned k;
};

static void assert_open(ifstream& f, const string& p)
{
	if (f.is_open())
		return;
	cerr << p << ": " << strerror(errno) << endl;
	exit(EXIT_FAILURE);
}

/** Read contig lengths. */
vector<unsigned> readContigLengths(const string& path)
{
	vector<unsigned> lengths;
	ifstream in(path.c_str());
	assert_open(in, path);

	assert(g_contigIDs.empty());
	string id;
	unsigned len;
	while (in >> id >> len) {
		in.ignore(numeric_limits<streamsize>::max(), '\n');
		(void)g_contigIDs.serial(id);
		assert(len >= opt::k);
		lengths.push_back(len - opt::k + 1);
	}
	assert(in.eof());
	assert(!lengths.empty());
	return lengths;
}