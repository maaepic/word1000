#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <sys/mman.h> // TODO: make _WIN32 compat layer
#include <sys/stat.h>

#include <stdio.h>
#include <fcntl.h>

// external xml library
// `git clone https://github.com/michaelrsweet/mxml; cd mxml; ./configure; make`
#include "mxml/mxml.h"

// global word number(s)
int Number_to_thicken = 0;
int Current_word_number = 0;

// make a deep node copy, only msword format elements supported
mxml_node_t* mxmlCopy(mxml_node_t* root, mxml_node_t* node)
{
	mxml_node_t* new1 = NULL;
	switch (mxmlGetType(node))
	{
		case MXML_TYPE_ELEMENT:
			new1 = mxmlNewElement(root, mxmlGetElement(node));
			break;
		case MXML_TYPE_TEXT: {
			bool whitespace = false;
			const char* text = mxmlGetText(node, &whitespace);
			new1 = mxmlNewText(root, whitespace, text);
			break;
		}
		default:
			perror("unexpected xml element"); // looks like invalid file format
			exit(9);
			break;
	}

	// copy attributes
	for (int i = 0; i < mxmlElementGetAttrCount(node); i++) {
		const char* name = 0;
		const char* value = mxmlElementGetAttrByIndex(node, i, &name);
		mxmlElementSetAttr(new1, name, value);
	}

	// copy children
	mxml_node_t* child = mxmlGetFirstChild(node);
	while (child) {
		mxmlCopy(new1, child);
		child = mxmlGetNextSibling(child);
	}

	return new1;
}


// walks xml structure, makes n-word thick (bold attribute)
bool walker(mxml_node_t* root)
{
	const char* rootname = mxmlGetElement(root); (void)rootname; // debug purpose
	mxml_node_t* node = mxmlGetFirstChild(root);
	while (node) {
		mxml_type_t type = mxmlGetType(node);
		if (type == MXML_TYPE_ELEMENT) {
			const char* name = mxmlGetElement(node);
			if (strcmp(name, "w:t") == 0) {
				assert (strcmp(rootname, "w:r") == 0); // msword format limitation

				mxml_node_t* text = mxmlGetFirstChild(node);
				assert (text && (mxmlGetType(text) == MXML_TYPE_TEXT)); // same assertion

				// we found a text, iterate over words
				int p = -1; // word number inside <w:r>
				while (text) {
					bool whitespace = true;
					const char* word = mxmlGetText(text, &whitespace);
					p++;

					if (word && *word) // we don't count whitespaces
						Current_word_number++;
					if (Current_word_number == Number_to_thicken) {
						assert (strcmp(rootname, "w:r") == 0); // just reminder

						mxml_node_t* sub;
						
						// extract "before word" part (delete all with word and after)
						if (p > 0) {
							mxml_node_t* prev = mxmlCopy(NULL, root);
							mxmlAdd(mxmlGetParent(root), MXML_ADD_BEFORE, root, prev);

							sub = mxmlFindPath(prev, "w:t");
							// note: msword has only one <w:t> inside <w:r>
							for (int i = 0; i < p; i++)
								sub = mxmlGetNextSibling(sub);
							while (sub) {
								mxml_node_t* sup = sub;
								sub = mxmlGetNextSibling(sub);
								mxmlDelete(sup);
							}
						}

						// extract "after word" part (delete all with word and before)
						mxml_node_t* next = mxmlCopy(NULL, root);
						sub = mxmlFindPath(next, "w:t");
						mxmlElementSetAttr(mxmlGetParent(sub), "xml:space", "preserve");
						for (int i = 0; i <= p; i++) {
							mxml_node_t* sup = sub;
							sub = mxmlGetNextSibling(sub);
							mxmlDelete(sup);
						}
						if (sub) // have more words after selected one in <w:r>
							mxmlAdd(mxmlGetParent(root), MXML_ADD_AFTER, root, next);
						else
							mxmlDelete(next);

						// remove all except selected word
						sub = mxmlGetFirstChild(node);
						mxmlElementSetAttr(mxmlGetParent(sub), "xml:space", "preserve");
						for (int i = 0; i < p; i++) {
							mxml_node_t* sup = sub;
							sub = mxmlGetNextSibling(sub);
							mxmlDelete(sup);
						}
						sub = mxmlGetNextSibling(sub);
						while (sub) {
							mxml_node_t* sup = sub;
							sub = mxmlGetNextSibling(sub);
							mxmlDelete(sup);
						}
						// last step:
						// 1. create text decoration element in not exist
						sub = mxmlFindPath(root, "w:rPr");
						if (!sub) {
							sub = mxmlNewElement(NULL, "w:rPr");
							mxmlAdd(root, MXML_ADD_BEFORE, node, sub);
						}
						// 2. mark selected word with "bold"
						mxmlNewElement(sub, "w:b");
						mxmlNewElement(sub, "w:bCs");

						return false; // done, we don't need to walk more
					}

					// continue words counting
					text = mxmlGetNextSibling(text);
				}
			}
		}

		if (!walker(node))
			return false;
		node = mxmlGetNextSibling(node);
	};
	return true;
}

// Usage: program <word number>
// put source document.xml into stdin
// and output fixed doc into stderr
int main(int argc, char** argv) {
	if (argc != 2) {
		printf("Usage: %s number\n", argv[0]);
		return 1;
	}

	Number_to_thicken = atoi(argv[1]);
	if (Number_to_thicken <= 0) {
		printf("error: number must be >= 1\n");
		return 2;
	}

	mxml_node_t * xml = mxmlLoadFd(NULL, NULL, STDIN_FILENO);
	if (!xml) {
		printf("error: invalid xml\n");
		return 3;
	}

	Current_word_number = 0; // setup
	walker(xml);
	mxmlSaveFd(xml, NULL, STDERR_FILENO);
	return 0;
}