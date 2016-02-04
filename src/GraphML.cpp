#include "GraphML.h"

#include "DirectedGraph.h"
#include "UndirectedGraph.h"

#include <tinyxml2.h>
#include <Table.h>

#include <cassert>
#include <map>
#include <iostream>

using namespace std;
using namespace tinyxml2;

GraphML::GraphML() : FileTypeHandler("GraphML", true) {
  addExtension("graphml");
}

std::shared_ptr<Graph>
GraphML::openGraph(const char * filename) {
  RenderMode mode = RENDERMODE_3D;

  XMLDocument doc;
  doc.LoadFile(filename);

  XMLElement * graphml_element = doc.FirstChildElement("graphml");
  assert(graphml_element);

  XMLElement * graph_element = graphml_element->FirstChildElement("graph");
  assert(graph_element);

  return createGraphFromElement(*graphml_element, *graph_element);  
}

std::shared_ptr<Graph>
GraphML::createGraphFromElement(XMLElement & graphml_element, XMLElement & graph_element) const {
  map<string, int> nodes_by_id;
  
  const char * edgedefault = graph_element.Attribute("edgedefault");
  bool directed = true;
  if (edgedefault && strcmp(edgedefault, "undirected") == 0) {
    directed = false;
  }
  std::shared_ptr<Graph> graph;
  if (directed) {
    graph = std::make_shared<DirectedGraph>();
    graph->setNodeArray(std::make_shared<NodeArray>());
    graph->getNodeArray().setNodeSizeMethod(SizeMethod(SizeMethod::SIZE_FROM_INDEGREE));
  } else {
    graph = std::make_shared<UndirectedGraph>();
    graph->setNodeArray(std::make_shared<NodeArray>());
    graph->getNodeArray().setNodeSizeMethod(SizeMethod(SizeMethod::SIZE_FROM_DEGREE));
  }

  auto & node_table = graph->getNodeArray().getTable();
  auto & edge_table = graph->getFaceData();
  
  auto & node_id_column = node_table.addTextColumn("id");
  auto & edge_id_column = edge_table.addTextColumn("id");
    
  XMLElement * key_element = graphml_element.FirstChildElement("key");
  for ( ; key_element ; key_element = key_element->NextSiblingElement("key") ) {
    const char * key_type = key_element->Attribute("attr.type");
    const char * key_id = key_element->Attribute("id");
    const char * for_type = key_element->Attribute("for");
    const char * name = key_element->Attribute("attr.name");
    
    assert(key_type && key_id && for_type);
    
    std::shared_ptr<table::Column> column;
    if (strcmp(key_type, "string") == 0) {
      column = std::make_shared<table::ColumnText>(key_id ? key_id : "");
    } else if (strcmp(key_type, "double") == 0 || strcmp(key_type, "float") == 0) {
      column = std::make_shared<table::ColumnDouble>(key_id ? key_id : "");
    } else if (strcmp(key_type, "int") == 0) {
      column = std::make_shared<table::ColumnInt>(key_id ? key_id : "");
    } else {
      assert(0);
    }

    if (strcmp(for_type, "node") == 0) {
      node_table.addColumn(column);
    } else if (strcmp(for_type, "edge") == 0) {
      edge_table.addColumn(column);
    } else {
      assert(0);
    }
  }
    
  bool is_complex = false;

  XMLElement * node_element = graph_element.FirstChildElement("node");
  for ( ; node_element ; node_element = node_element->NextSiblingElement("node") ) {
    const char * node_id_text = node_element->Attribute("id");
    assert(node_id_text);

    int node_id = graph->addNode();
    graph->addEdge(node_id, node_id);
    node_id_column.setValue(node_id, node_id_text);
    nodes_by_id[node_id_text] = node_id + 1;
    
    XMLElement * data_element = node_element->FirstChildElement("data");
    for ( ; data_element ; data_element = data_element->NextSiblingElement("data") ) {
      const char * key = data_element->Attribute("key");
      assert(key);
      const char * text = data_element->GetText();
      if (text) {
	node_table[key].setValue(node_id, text);
      }
    }

    graph->getNodeArray().updateNodeAppearanceSlow(node_id);

    XMLElement * nested_graph_element = node_element->FirstChildElement("graph");
    if (nested_graph_element) {
      auto nested_graph = createGraphFromElement(graphml_element, *nested_graph_element);
      assert(nested_graph.get());
      is_complex = true;
      graph->getNodeArray().setNestedGraph(node_id, nested_graph);
    }
  }

  graph->randomizeGeometry();
  graph->setComplexGraph(is_complex);
  graph->setHasSubGraphs(is_complex);

  XMLElement * edge_element = graph_element.FirstChildElement("edge");
  for ( ; edge_element ; edge_element = edge_element->NextSiblingElement("edge") ) {
    const char * edge_id_text = edge_element->Attribute("id");
    const char * source = edge_element->Attribute("source");
    const char * target = edge_element->Attribute("target");
    assert(source && target);

    if (strcmp(source, target) == 0) {
      cerr << "GraphML: skipping self link for " << source << endl;
      continue;
    }
    
    int source_node = nodes_by_id[source];
    int target_node = nodes_by_id[target];

    if (source_node && target_node) {
      int edge_id;
      if (directed) {
	edge_id = graph->addEdge(source_node - 1, target_node - 1);
	if (edge_id_text && strlen(edge_id_text) != 0) {
	  edge_id_column.setValue(edge_id, edge_id_text);
	}
      } else {
	edge_id = graph->addFace(-1);
	graph->addEdge(source_node - 1, target_node - 1, edge_id);
	graph->addEdge(target_node - 1, source_node - 1, edge_id);       
      }

      XMLElement * data_element = edge_element->FirstChildElement("data");
      
      for ( ; data_element ; data_element = data_element->NextSiblingElement("data") ) {
	const char * key = data_element->Attribute("key");
	assert(key);
	const char * text = data_element->GetText();
	if (text) {
	  edge_table[key].setValue(edge_id, text);
	}
      }
    } else {
      if (!source_node) {
	// cerr << "cannot find node " << source << endl;
      }
      if (!target_node) {
	// cerr << "cannot find node " << target << endl;
      }
    }
  }

  return graph;      
}

bool
GraphML::saveGraph(const Graph & graph, const std::string & filename) {
  XMLDocument doc;
  doc.LinkEndChild(doc.NewDeclaration());

  XMLElement * graphml_element = doc.NewElement("graphml");
  graphml_element->SetAttribute("xmlns", "http://graphml.graphdrawing.org/xmlns");

  bool directed = graph.isDirected();
  auto & node_table = graph.getNodeArray().getTable();
  auto & edge_table = graph.getFaceData();

  for (auto & col : node_table.getColumns()) {
    XMLElement * key_element = doc.NewElement("key");
    key_element->SetAttribute("attr.name", col.first.c_str());
    key_element->SetAttribute("id", col.first.c_str());
    key_element->SetAttribute("attr.type", col.second->getTypeText());
    key_element->SetAttribute("for", "node");
    graphml_element->LinkEndChild(key_element);    
  }

  for (auto & col : edge_table.getColumns()) {
    XMLElement * key_element = doc.NewElement("key");
    key_element->SetAttribute("attr.name", col.first.c_str());
    key_element->SetAttribute("id", col.first.c_str());
    key_element->SetAttribute("attr.type", col.second->getTypeText());
    key_element->SetAttribute("for", "edge");
    graphml_element->LinkEndChild(key_element);    
  }

  XMLElement * graph_element = doc.NewElement("graph");
  graphml_element->LinkEndChild(graph_element);

  if (!directed) {
    graph_element->SetAttribute("edgedefault", "undirected");
  }

  for (unsigned int i = 0; i < graph.getNodeArray().size(); i++) {
    XMLElement * node_element = doc.NewElement("node");
    string node_id = "n" + to_string(i);
    node_element->SetAttribute("id", node_id.c_str());    

    for (auto & col : node_table.getColumns()) {
      XMLElement * data_element = doc.NewElement("data");
      data_element->SetAttribute("key", col.first.c_str());
      data_element->LinkEndChild(doc.NewText(col.second->getText(i).c_str()));
      node_element->LinkEndChild(data_element);
    }
    
    graph_element->LinkEndChild(node_element);
  }

  for (unsigned int i = 0; i < graph.getFaceCount(); i++) {
    XMLElement * edge_element = doc.NewElement("edge");

#if 0
    auto edge = graph.getEdgeAttributes(i);
    string node_id1 = "n" + to_string(edge.tail);
    string node_id2 = "n" + to_string(edge.head);

    // timestamp, sentiment
    
    edge_element->SetAttribute("source", node_id1.c_str());
    edge_element->SetAttribute("target", node_id2.c_str());
#endif

    for (auto & col : edge_table.getColumns()) {
      XMLElement * data_element = doc.NewElement("data");
      data_element->SetAttribute("key", col.first.c_str());
      data_element->LinkEndChild(doc.NewText(col.second->getText(i).c_str()));
      edge_element->LinkEndChild(data_element);
    }
    
    graph_element->LinkEndChild(edge_element);
  }

  doc.LinkEndChild(graphml_element);
  doc.SaveFile(filename.c_str());
  
  return true;
}
