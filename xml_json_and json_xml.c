#include <stdio.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <json/json.h>
#include <libxml/tree.h>
#include <libxml/parser.h> 

/*
XML_ELEMENT_NODE = 1
XML_ATTRIBUTE_NODE = 2
*/

typedef struct Node
{
	json_object *jsonObject;
	const xmlChar *name;
	xmlNode *node;
	struct Node *next;
	struct Node *previous;
}Node;

typedef struct List
{
	Node *head;
	Node *tail;
	int nodeIndex;
}List;


int objectCounter = 0; /* json object counter */
int elementCounter = 0; /* element counter */
int jsonArrayCounter = 0; /* json array counter */

/* functions of XML Paring */
bool isArray(xmlNode *node);
void CreateList(List *list);
int findElementSize(xmlNode *node);
xmlNode* findNode(xmlNode *xmlnode);
json_object* findJsonObject(List *list, const xmlChar *name);
bool findSibling(List *list, const xmlChar *name, xmlNode *node);
void addToList(List *list, json_object *object, const xmlChar *name, xmlNode *node);
void convertStringToJson(const xmlChar *char1, xmlChar *char2, json_object *object);
void addXmlToJson(xmlNode *node, List *obj_list, json_object *json_obj[], List *object_arr, json_object *json_arr[]);

/* functions of Json Parsing */
void JsonParse(json_object *jsonObject, xmlNode *xmlnode);
void JsonParseArray(json_object *jsonObject, char *key, xmlNode *xmlnode);


void main(int argc, char *argv[])
{
	if(strstr(argv[1], ".json") != NULL)
	{
		xmlDocPtr document = xmlNewDoc(BAD_CAST "1.0");
		json_object *jsonObject = json_object_from_file(argv[1]);

		xmlNodePtr rootNode = xmlNewNode(NULL, BAD_CAST "root");
		xmlDocSetRootElement(document, rootNode);

		JsonParse(jsonObject, rootNode);

		xmlSaveFormatFileEnc("onur.xml", document, "UTF-8", 1);
		
		xmlFreeDoc(document);
		xmlCleanupParser();
		xmlMemoryDump();
	}

	if (strstr(argv[1], ".xml") != NULL)
	{
		xmlDoc *document = NULL; xmlNode *rootNode = NULL;
		document = xmlReadFile(argv[1], NULL, 0);

		if (document != NULL)
		{
			rootNode = xmlDocGetRootElement(document);
			
			List objectList; CreateList(&objectList);
			List arrayList; CreateList(&arrayList);

			json_object *jsonObject = json_object_new_object();
			
			elementCounter = findElementSize(rootNode);
			json_object *object[elementCounter];
			json_object *array[elementCounter];

			int i;
			for(i = 0; i < elementCounter; i++)
			{
				array[i] = json_object_new_array();
				object[i] = json_object_new_object();
			}

			xmlNode *tempNode = rootNode;	
			while (tempNode->type != 1)
			{
				tempNode = tempNode->next;
			}

			if(tempNode->properties != NULL || tempNode->children != NULL)
			{
				json_object *obj = json_object_new_object();
				json_object_object_add(jsonObject, tempNode->name, obj);
				addToList(&objectList, obj, tempNode->name, tempNode);

				if(tempNode->properties != NULL)
				{
					xmlAttr *xmlAttribute = NULL;

					for(xmlAttribute = tempNode->properties; xmlAttribute; xmlAttribute = xmlAttribute->next)
					{
						if(xmlAttribute->type == 2) convertStringToJson(xmlAttribute->name, xmlAttribute->children->content, obj);
					}
				}
			}

			else convertStringToJson(tempNode->name, tempNode->children->content, jsonObject); /* have not attributes and children */

			addXmlToJson(findNode(tempNode->children), &objectList, object, &arrayList, array);

			json_object_to_file("onur.json", jsonObject);
			xmlFreeDoc(document);
		}
		xmlCleanupParser();
	}
}


void CreateList(List *list)
{
	/* initilaize for list */
	list->head = (Node*)malloc(sizeof(Node));
	list->head->previous = NULL;
	list->head->next = NULL;
	list->tail = list->head;
	list->nodeIndex = 0;
}


int findElementSize(xmlNode *node)
{
	/* find element size for array and object arrays */
	xmlNode *tempNode = NULL;

	for (tempNode = node; tempNode; tempNode = tempNode->next) 
	{
		if (tempNode->type == 1) elementCounter++;

		findElementSize(tempNode->children);
	}
	return elementCounter;
}


json_object* findJsonObject(List *list, const xmlChar *name)
{
	/* find json object from list */

	Node *temp = list->head;
	json_object *object = NULL;

	if(list->nodeIndex == 0) return NULL;

	while(temp != NULL)
	{
		if(strcmp(temp->name, name) == 0) object = temp->jsonObject;

		temp = temp->next;
	}
	return object;
}


xmlNode* findNode(xmlNode *xmlnode)
{
	xmlNode *tempNode = NULL;

	for (tempNode = xmlnode; tempNode; tempNode = tempNode->next) 
	{
		if (tempNode->type == 1) return tempNode;
	}

	return NULL;
}


bool isFindSibling(List *list, const xmlChar *name, xmlNode *node)
{
	/* Arraye eleman eklerken, eklenen nodenin kardeşleri ile aynı isimde olup olmadığını kontrol ediyor. 
	list'i next ve previous olarka geziyor.*/

	Node *currentNode = list->head;

	if(list->nodeIndex == 0) return false;

	while(currentNode != NULL)
	{
		if(strcmp(currentNode->name, name) == 0)
		{
			xmlNode *tempNode;

			for(tempNode = currentNode->node->prev; tempNode; tempNode = tempNode->prev)
			{
				if(tempNode == node) return true;
			}

			for(tempNode = currentNode->node->next; tempNode; tempNode = tempNode->next)
			{
				if(tempNode == node) return true;
			}
		}
		currentNode = currentNode->next;
	}
	return false;
}


void addToList(List *list, json_object *object, const xmlChar *name, xmlNode *node)
{
	/* firstly create a node according to attributes, after add to list */

	if(list->nodeIndex == 0)
	{
		list->head->jsonObject = object;
		list->head->name = name;
		list->head->node = node;
		list->nodeIndex++;
		
		return;
	}

	Node *_newNode = (Node*)malloc(sizeof(Node));
	_newNode->jsonObject = object;
	_newNode->name = name;
	_newNode->node = node;

	list->tail->next = _newNode;
	_newNode->previous = list->tail;
	list->tail = _newNode;
	list->tail->next = NULL;
	list->nodeIndex++;
}


void convertStringToJson(const xmlChar *char1, xmlChar *char2, json_object *object)
{
	/*Burayı fonksiyon yapmamın sebebi : json kütüphanesi güncellendiğinden console'da warningleri azaltmak için */
	json_object_object_add(object, char1, json_object_new_string(char2));
}



 void addXmlToJson(xmlNode *node, List *obj_list, json_object *json_obj[], List *object_arr, json_object *json_arr[])
{
	xmlNode *currentNode = NULL;

	for(currentNode = node; currentNode; currentNode = currentNode->next)
	{
		if (currentNode->type == 2 || currentNode->type == 1)
		{
			if(currentNode->properties != NULL || findNode(currentNode->children) != NULL)
			{
				if(findJsonObject(obj_list, (findNode(currentNode->parent))->name) != NULL && findNode(currentNode->parent) != NULL) /* obje olabilir */
				{
					if(isArray(currentNode) == true) /* array kontrolü yapılıyor ve array olduğu doğrulanıyor */
					{
						if(isFindSibling(object_arr, currentNode->name, currentNode) == false || findJsonObject(object_arr, currentNode->name) == NULL)
						{
							json_object_array_add(json_arr[jsonArrayCounter], json_obj[objectCounter]);
							json_object_object_add(findJsonObject(obj_list, (findNode(currentNode->parent))->name), currentNode->name, json_arr[jsonArrayCounter]);
							
							addToList(object_arr, json_arr[jsonArrayCounter], currentNode->name, currentNode); /* araylerin listine ekleniyor */
							jsonArrayCounter++;
						}

						else 
						{
							json_object_array_add(findJsonObject(object_arr, currentNode->name), json_obj[objectCounter]);
						}
					}

					else /* eğer array değilse */
					{
						json_object_object_add(findJsonObject(obj_list, (findNode(currentNode->parent))->name), currentNode->name, json_obj[objectCounter]);
					}
						
					addToList(obj_list, json_obj[objectCounter], currentNode->name, currentNode); /* objelerin listine ekleniyor */
					objectCounter++;
				}
			}

			else /* çocuğu ya da attributesi yoksa */
			{
				if(isArray(currentNode) == true) /* array ise --->control */
				{
					if(isFindSibling(object_arr, currentNode->name, currentNode) == false || findJsonObject(object_arr, currentNode->name) == NULL)
					{
						json_object_array_add(json_arr[jsonArrayCounter], json_object_new_string(currentNode->children->content));
						json_object_object_add(findJsonObject(obj_list, (findNode(currentNode->parent))->name), currentNode->name, json_arr[jsonArrayCounter]);

						addToList(object_arr, json_arr[jsonArrayCounter], currentNode->name, currentNode);
						jsonArrayCounter++;
					}
					
					else
					{
						json_object_array_add(findJsonObject(object_arr, currentNode->name), json_object_new_string(currentNode->children->content));
					}
				}

				else /* array değilse --> string */
				{
					convertStringToJson(currentNode->name, currentNode->children->content, findJsonObject(obj_list, (findNode(currentNode->parent))->name));
				}
			}

			if(currentNode->properties != NULL) /* attribute varsa */
			{
				xmlAttr *attribute = NULL;

				for(attribute = currentNode->properties; attribute; attribute = attribute->next)
				{
					if(attribute->type == 2)
					{
						convertStringToJson(attribute->name, attribute->children->content, findJsonObject(obj_list, currentNode->name));
					}
				}
			}
		}
		addXmlToJson(currentNode->children, obj_list, json_obj, object_arr, json_arr);
	}
}


bool isArray(xmlNode *node)
{
	xmlNode *temp = NULL;

	for(temp = node->prev; temp; temp = temp->prev)
	{
		if(temp->type == 1 && strcmp(node->name, temp->name) == 0) return true;
	}

	for(temp = node->next; temp; temp = temp->next)
	{
		if(temp->type == 1 && strcmp(node->name, temp->name) == 0) return true;
	}

	return false;
}


void JsonParse(json_object *jsonObject, xmlNode *xmlnode)
{
	enum json_type jsonType;

	json_object_object_foreach(jsonObject, key, value)
	{
		jsonType = json_object_get_type(value);

		if(jsonType == json_type_object) /* object */
		{
			xmlNode* node = xmlNewChild(xmlnode, NULL, key, NULL);
			JsonParse(json_object_object_get(jsonObject, key), node);
		}

		else if (jsonType == json_type_array) JsonParseArray(jsonObject, key, xmlnode);

		else xmlNewChild(xmlnode, NULL, key, json_object_get_string(value));
	}
}


void JsonParseArray(json_object *jsonObject, char *key, xmlNode *xmlnode)
{
	json_object *jsonArray = json_object_object_get(jsonObject, key);

	int i;
	for(i = 0; i < json_object_array_length(jsonArray); i++)
	{
		json_object *jsonValue = json_object_array_get_idx(jsonArray, i);

		if(json_object_get_type(jsonValue) == json_type_object) /* object */
		{
			xmlNode* node = xmlNewChild(xmlnode, NULL, key, NULL);
			JsonParse(jsonValue, node);
		}

		else if(json_object_get_type(jsonValue) == json_type_array) JsonParseArray(jsonValue, NULL, xmlnode);

		else xmlNewChild(xmlnode, NULL, key, json_object_get_string(jsonValue));
	}
}
