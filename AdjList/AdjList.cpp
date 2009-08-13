#include "FastaReader.h"
#include "PackedSeq.h"
#include "PairUtils.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <cstring> // for strerror
#include <fstream>
#include <functional>
#include <iterator>
#include <getopt.h>
#include <iostream>

using namespace std;

#define PROGRAM "AdjList"

static const char *VERSION_MESSAGE =
PROGRAM " (ABySS) " VERSION "\n"
"Written by Jared Simpson and Shaun Jackman.\n"
"\n"
"Copyright 2009 Canada's Michael Smith Genome Science Centre\n";

static const char *USAGE_MESSAGE =
"Usage: " PROGRAM " [OPTION]... [FILE]...\n"
"Find all contigs that overlap by exactly k-1 bases. Contigs may be read\n"
"from FILE(s) or standard input. Output is written to standard output.\n"
"\n"
"  -k, --kmer=KMER_SIZE  k-mer size\n"
"      --adj             output the results in adj format [DEFAULT]\n"
"      --dot             output the results in dot format\n"
"  -v, --verbose         display verbose output\n"
"      --help            display this help and exit\n"
"      --version         output version information and exit\n"
"\n"
"Report bugs to <" PACKAGE_BUGREPORT ">.\n";

/** Enumeration of output formats */
enum format { ADJ, DOT };

namespace opt {
	static int k;
	static int overlap;

	/** Output formats */
	static int format;

	static int verbose;
	extern bool colourSpace;
}

static const char* shortopts = "k:v";

enum { OPT_HELP = 1, OPT_VERSION };

static const struct option longopts[] = {
	{ "kmer",    required_argument, NULL, 'k' },
	{ "adj",     no_argument,       &opt::format, ADJ },
	{ "dot",     no_argument,       &opt::format, DOT },
	{ "verbose", no_argument,       NULL, 'v' },
	{ "help",    no_argument,       NULL, OPT_HELP },
	{ "version", no_argument,       NULL, OPT_VERSION },
	{ NULL, 0, NULL, 0 }
};

/** A contig ID and its end sequences. */
struct ContigEndSeq {
	ContigID id;
	unsigned length;
	PackedSeq l;
	PackedSeq r;
	ContigEndSeq(const ContigID& id, unsigned length,
			const PackedSeq& l, const PackedSeq& r)
		: id(id), length(length), l(l), r(r) { }
};

static void readContigs(string path, vector<ContigEndSeq>* pContigs)
{
	unsigned count = 0;
	FastaReader in(path.c_str());
	for (FastaRecord rec; in >> rec;) {
		const Sequence& seq = rec.seq;
		if (count++ == 0) {
			// Detect colour-space contigs.
			opt::colourSpace = isdigit(seq[0]);
		} else {
			if (opt::colourSpace)
				assert(isdigit(seq[0]));
			else
				assert(isalpha(seq[0]));
		}

		PackedSeq seql = seq.substr(seq.length() - opt::overlap,
				opt::overlap);
		PackedSeq seqr = seq.substr(0, opt::overlap);
		pContigs->push_back(ContigEndSeq(rec.id, seq.length(),
					seql, seqr));
	}
	assert(in.eof());
}

template<class T> static bool predTrue(T o)
{
	(void)o;
	return true;
}

template<typename T, class P> const T& second(const P& o)
{
	return o.second;
}

int main(int argc, char** argv)
{
	bool die = false;
	for (int c; (c = getopt_long(argc, argv,
					shortopts, longopts, NULL)) != -1;) {
		istringstream arg(optarg != NULL ? optarg : "");
		switch (c) {
			case '?': die = true; break;
			case 'k': arg >> opt::k; break;
			case 'v': opt::verbose++; break;
			case OPT_HELP:
				cout << USAGE_MESSAGE;
				exit(EXIT_SUCCESS);
			case OPT_VERSION:
				cout << VERSION_MESSAGE;
				exit(EXIT_SUCCESS);
		}
	}

	if (opt::k <= 0) {
		cerr << PROGRAM ": " << "missing -k,--kmer option\n";
		die = true;
	}

	if (die) {
		cerr << "Try `" << PROGRAM
			<< " --help' for more information.\n";
		exit(EXIT_FAILURE);
	}

	opt::overlap = opt::k - 1;

	vector<ContigEndSeq> contigs;
	if (optind < argc) {
		for_each(argv + optind, argv + argc,
				bind2nd(ptr_fun(readContigs), &contigs));
	} else
		readContigs("-", &contigs);

	typedef multimap<PackedSeq, SimpleEdgeDesc> KmerMap;
	KmerMap ends[2];
	for (vector<ContigEndSeq>::const_iterator i = contigs.begin();
			i != contigs.end(); ++i) {
		ends[0].insert(make_pair(i->l,
					SimpleEdgeDesc(i->id, false)));
		ends[1].insert(make_pair(reverseComplement(i->l),
					SimpleEdgeDesc(i->id, true)));
		ends[1].insert(make_pair(i->r,
					SimpleEdgeDesc(i->id, false)));
		ends[0].insert(make_pair(reverseComplement(i->r),
					SimpleEdgeDesc(i->id, true)));
	}

	ostream& out = cout;
	if (opt::format == DOT)
		out << "digraph adj {\n";

	int numVerts = 0;
	int numEdges = 0;
	for (vector<ContigEndSeq>::const_iterator i = contigs.begin();
			i != contigs.end(); ++i) {
		const ContigID& id = i->id;

		if (opt::format == ADJ)
			out << id << ' ' << i->length;

		for (unsigned idx = 0; idx < 2; idx++) {
			const PackedSeq& seq = idx == 0 ? i->l : i->r;
			pair<KmerMap::const_iterator, KmerMap::const_iterator>
				edges = ends[!idx].equal_range(seq);

			switch (opt::format) {
			  case ADJ:
				out << " [ ";
				transform(edges.first, edges.second,
						ostream_iterator<SimpleEdgeDesc>(out, " "),
						second<KmerMap::mapped_type,
							KmerMap::value_type>);
				out << ']';
				break;
			  case DOT:
				out << '"' << id << (idx ? '-' : '+') << "\" [len="
					<< i->length << "];\n"
					<< '"' << id << (idx ? '-' : '+') << '"';
				if (edges.first != edges.second) {
					out << " -> {";
					for (KmerMap::const_iterator it = edges.first;
							it != edges.second; ++it)
						out << " \"" << it->second.contig
							<< (idx != it->second.isRC ? '-' : '+')
							<< '"';
					out << " }";
				}
				out << ";\n";
				break;
			}
			numEdges += count_if(edges.first, edges.second,
					predTrue<KmerMap::value_type>);
		}
		if (opt::format == ADJ)
			out << '\n';
		numVerts++;
	}

	if (opt::format == DOT)
		out << "}\n";

	if (opt::verbose > 0)
		cerr << "vertices: " << numVerts << " "
			"edges: " << numEdges << endl;
}
