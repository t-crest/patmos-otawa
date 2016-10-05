/*
 *	$Id$
 *	BBRatioDisplayer processor interface
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2008, IRIT UPS.
 * 
 *	OTAWA is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	OTAWA is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with OTAWA; if not, write to the Free Software 
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <elm/system/System.h>
#include <otawa/ipet.h>
#include <otawa/util/BBRatioDisplayer.h>
#include <otawa/hard/CacheConfiguration.h>
#include <otawa/dcache/features.h>

#include <map>

namespace otawa {

// Private identifier
otawa::Identifier<int> BBRatioDisplayer::SUM("", 0);


/**
 * @class BBRatioDisplayer
 * This class output statistics for each basic block in the WCET as a table
 * in a textual form on the standard output.
 * The following columns are available:
 * @li @c ADDRESS -- address of the BB,
 * @li @c NUM -- BB number (in the CFG),
 * @li @c SIZE -- size in bytes of the BB,
 * @li @c TIME -- time in cycles of one basic block execution,
 * @li @c COUNT -- number of times the basic block is executed in the WCET,
 * @li @c RATIO -- ratio occupied in the WCET by the cumulative execution of the BB,
 * @li @c FUNCTION -- function (if any) containing the BB.
 * 
 * In addition, a line is devoted to the same statistics for a whole function
 * and the last line sum up this column for the WCET.
 * 
 * @warning Due to overlapping of BB execution time on pipelined processors,
 * the sum of all basic blocks ratio may be a bit greater than 100%.
 * 
 * @par Required features
 * @li @ref ipet::WCET_FEATURE
 * @li @ref ipet::ASSIGNED_VARS_FEATURE
 *
 * @par Configuration
 * @li @ref otawa::BBRatioDisplay::TO_FILE
 * @li @ref otawa::BBRatioDisplay::PATH
 */

p::declare BBRatioDisplayer::reg = p::init("otawa::BBRatioDisplayer", Version(1, 0, 1))
	.require(ipet::WCET_FEATURE)
	.require(ipet::ASSIGNED_VARS_FEATURE)
	.maker<BBRatioDisplayer>();

/**
 * Build the processor.
 */
BBRatioDisplayer::BBRatioDisplayer(AbstractRegistration& r)
: BBProcessor(r), path(""), to_file(false), stream(0), line(false) {
}


/**
 */
void BBRatioDisplayer::configure(const PropList& props) {
	BBProcessor::configure(props);
	path = PATH(props);
	to_file = TO_FILE(props);
}


/**
 */
void BBRatioDisplayer::setup(WorkSpace *ws) {

	// source available ?
	if(ws->isProvided(SOURCE_LINE_FEATURE))
		line = true;
	if(logFor(LOG_PROC))
		log << "\tsource/line information " << (line ? "" : "not ") << "available\n";

	// prepare the output
	if(!path && to_file)
		path = _ << ENTRY_CFG(ws)->label() << ".ratio";
	if(path) {
		if(logFor(LOG_PROC))
			log << "\toutputting to " << path << io::endl;
		try {
			stream = elm::system::System::createFile(path);
		}
		catch(elm::system::SystemException& exn) {
			throw ProcessorException(*this, _ << "cannot open \"" << path << "\"");
		}
		out.setStream(*stream);
	}

	// prepare the work
	wcet = ipet::WCET(ws);
	system = ipet::SYSTEM(ws);
	out << "ADDRESS\tNUM\tSIZE\tTIME\tCOUNT\tCACHE\tDCACHE\tTOTAL\tRATIO\tFUNCTION";
	if(line)
		out << "\tLINE";
	out << io::endl;
}

static std::map<address_t, unsigned int> addrTimes;
static std::map<address_t, unsigned int> addrCache;
static std::map<address_t, unsigned int> addrDCache;

/**
 */
void BBRatioDisplayer::processCFG(WorkSpace *fw, CFG *cfg) {
	BBProcessor::processCFG(fw, cfg);
	out << "TOTAL FUNCTION\t\t"
		<< SUM(cfg) << '\t'
		<< (int)system->valueOf(ipet::VAR(cfg->entry())) << '\t'
		<< (float)SUM(cfg) * 100 / wcet << "%\t\t"
		<< cfg->label() << io::endl;
	out << "\n-----------\n";
	out << "ADDRESS\tCACHE\tDCACHE\tTIME\tRATIO\tFUNCTION\n";
	unsigned int accu_cache = 0, accu_dcache = 0;
	for(std::map<address_t, unsigned int>::const_iterator i(addrTimes.begin()), ie(addrTimes.end()); i != ie; i++)
	{
		out << i->first << "\t" << addrCache[i->first] << "\t" << addrDCache[i->first] << "\t" << i->second << "\t" << (float)i->second * 100 / wcet << "%\t" << cfg->label() << "\n";
		accu_cache += addrCache[i->first];
		accu_dcache += addrDCache[i->first];
	}
	out << "Total\t" << accu_cache << "\t" << accu_dcache << "\t" << wcet << "\t" << "\n";
}


/**
 */
void BBRatioDisplayer::processBB(WorkSpace *fw, CFG *cfg, BasicBlock *bb) {
	if(bb->isEnd())
		return;

	// accumulate instruction cache miss costs.
	int cache_total = 0;
	genstruct::AllocatedTable<LBlock *> *lbs = BB_LBLOCKS(bb);
	if (lbs)
	{
		const hard::Cache *inst_cache = hard::CACHE_CONFIGURATION(fw)->instCache();
		int cache_penalty = inst_cache->missPenalty();
		for(int i = 0; i < lbs->count(); i++)
		{
			LBlock *lb = lbs->get(i);
			ilp::Var *miss_var = MISS_VAR(lb);
			cache_total += (int)system->valueOf(miss_var) * cache_penalty;
		}
	}

	// accumulate data cache miss costs.
	const hard::Cache *data_cache = hard::CACHE_CONFIGURATION(fw)->dataCache();
	int dcache_penalty = data_cache->missPenalty();
	int dcache_total = 0;
	Pair<int, dcache::BlockAccess *> ab = dcache::DATA_BLOCKS(bb);
	for(int j = 0; j < ab.fst; j++) {
		dcache::BlockAccess& b = ab.snd[j];
		ilp::Var *miss_var = dcache::MISS_VAR(b);
		dcache_total += (int)system->valueOf(miss_var) * dcache_penalty;
	}

	int count = (int)system->valueOf(ipet::VAR(bb)),
		time = ipet::TIME(bb),
		total = time * count + cache_total + dcache_total;
	SUM(cfg) = SUM(cfg) + total;
	addrTimes[bb->address()] += total;
	addrCache[bb->address()] += cache_total;
	addrDCache[bb->address()] += dcache_total;
	out << bb->address() << '\t'
		<< bb->number() << '\t'
		<< bb->size() << '\t'
		<< time << '\t'
		<< count << '\t'
		<< cache_total << '\t'
		<< dcache_total << '\t'
		<< total << '\t'
		<< (float)total * 100 / wcet << "%\t"
		<< cfg->label();
	
	// Line display
	if(line) {
		out << '\t';
		bool one = false;
		Pair<cstring, int> old = pair(cstring(""), 0);
		for(Address addr = bb->address(); addr.offset() < bb->topAddress().offset(); addr = addr + 1) {
			Option<Pair<cstring, int> > res = fw->process()->getSourceLine(addr);
			if(res) {
				if((*res).fst != old.fst) {
					old = *res;
		 			if(one)
		 				out << ", ";
		 			else
		 				one = true;
		 			out << old.fst << ':' << old.snd;
				}
				else if((*res).snd != old.snd) {
					old = *res;
					out << ',' << old.snd;
				}
			} 
		}
	}
	
	// End of line
	out << io::endl; 
}


/**
 */
void BBRatioDisplayer::cleanup(WorkSpace *ws) {
	if(stream)
		delete stream;
}


/**
 * Configure the @ref BBRatiodisplayer to perform its output to a file.
 * If no @ref BBRatioDisplayer::PATH is given, the file name is obtained
 * from the entry function name and postfixed with ".ratio".
 */
Identifier<bool> BBRatioDisplayer::TO_FILE("otawa::BBRatioDisplayer::TO_FILE", false);


/**
 * Configure the @ref BBRatioDisplayer to perform its output to the named file.
 */
Identifier<elm::system::Path> BBRatioDisplayer::PATH("otawa::BBRatioDisplayer::PATH", "");


} // otawa
