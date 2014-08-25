#include "FlowGraph.h"
#include "Node_t.h"
#include "Edge_t.h"
#include "../console.h"

namespace fa{
	
		FlowGraph::FlowGraph(void){

		}
		FlowGraph::~FlowGraph(void){

		}

		bool FlowGraph::insertNode( Node_t* node )
		{
// 			if (!contains(nodes,(duint)node->vaddr))
// 			{
// 				return (nodes.insert(std::make_pair<duint,Node_t*>(node->vaddr,node))).second;
// 			}
			return true;
			
		}
		bool FlowGraph::insertEdge(Node_t* startNode, Node_t* endNode, EdgeType currentEdgeType){
			// until here a node just contains an address!
			// we search for these address to get most updated information (like incoming edges)
			const duint s = startNode->vaddr;
			const duint e = endNode->vaddr;

			std::map<duint,Node_t*>::iterator it = nodes.find(s);
			std::map<duint,Node_t*>::iterator it2 = nodes.find(e);
			dprintf("find node at "fhex" \n",s);
			dprintf("find node at "fhex" \n",e);
			if(it == nodes.end()){
				nodes.insert(std::make_pair(s,startNode));
				dprintf("create node at "fhex" \n",s);
				it = nodes.find(s);
			}

			if(it2 == nodes.end()){
				nodes.insert(std::make_pair(e,endNode));
				dprintf("create node at "fhex" \n",e);
				it2 = nodes.find(e);
			}

				
			
			dprintf("edge iter "fhex" to "fhex"\n",(*(it->second)).vaddr,(*(it2->second)).vaddr);
			

			Edge_t *edge = new Edge_t(&*(it->second),&*(it2->second),currentEdgeType);
			edge->start = &*(it->second);
			edge->end = &*(it2->second);

			it->second->outEdge = edge;
			it2->second->inEdges.insert(edge);

			

			bool ans = edges.insert(std::make_pair<duint,Edge_t*>(s,edge)).second;
			//std::map<duint,Edge_t*>::iterator ee = edges.find(s);

			dprintf(" an edge was insered from  "fhex" to "fhex"\n",edge->start->vaddr, edge->end->vaddr);
			for(std::set<Edge_t*>::iterator eit = edge->end->inEdges.begin(); eit != edge->end->inEdges.end();eit++){
				dprintf(" end edges has incoming from   "fhex"\n", (*eit)->start->vaddr);
			}


			
			return true;
		}
		

		void FlowGraph::clean()
		{
			// first delete all edges whoses aks for it
			std::map<duint,Edge_t*>::iterator e = edges.begin();
			while(e != edges.end()) {
				std::map<duint,Edge_t*>::iterator current = e++;
				if((*current->second).askForRemove){
					delete current->second;
					edges.erase(current);
				}
			}


			// find and delete isolated nodes, i.e. nodes without incoming and outgoing edges
			std::map<duint,Node_t*>::iterator n = nodes.begin();
			while(n != nodes.end()) {
				std::map<duint,Node_t*>::iterator current = n++;
				if( (!(*current->second).outEdge)  && ((*current->second).inEdges.size()==0) ){
					delete current->second;
					nodes.erase(current);
				}
			}
		}
		bool FlowGraph::find(const duint va , Node_t* node)
		{
			if(contains(nodes,va)){
				node = nodes.at(va);
				return true;
			}
			return false;
// 
// 			// try to find a node
// 			std::map<duint,Node_t*>::iterator it = nodes.find(va);
// 			if(it == nodes.end())
// 				return false;
// 			dprintf("**found node   "fhex" \n",it->second->vaddr);
// 			node = ((it->second));
// 			dprintf("**found node   "fhex" \n",node->vaddr);
// 			return true;
		}



}
