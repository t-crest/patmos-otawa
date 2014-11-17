/*
 *	$Id$
 *	EGSolver class.
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2007, IRIT UPS.
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

#include "EGSolver.h"

using namespace otawa;
using namespace otawa::exegraph2;


void EGGenericSolver::solve(ExecutionGraph *graph) {
	EGScenariiList *scenarii_list = _scenario_builder->build(_ws, graph);
	elm::cout << "scenarii list:\n";
	scenarii_list->dump();
	//delete scenarii_list; //FIXME
}
