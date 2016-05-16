// -*- compile-command: "g++ -std=c++11 -Wall -Werror -Weffc++ -pedantic -ggdb -o hlnode hlnode.cpp"  -*-

/** @file */

#include <stdio.h>
#include <string.h>
#include "hlnode.hpp"

HLNode::HLNode(const char *tag, const char *value, HLNode *parent)
   : m_parent(parent), m_child(nullptr), m_sibling(nullptr),
     m_tag(save_str(tag)), m_value(save_str(value))
{
}

HLNode::~HLNode()
{
   // delete down, then right:
   delete m_child;
   delete m_sibling;

   // delete strings
   delete [] m_tag;
   delete [] m_value;
}

/**
 * @brief Returns the last of the string of siblings.
 *
 * This function is mainly for adding a node to an existing tree.
 * The best practice for intial tree-building is to use recursion
 * to process children and looping to process siblings so as to
 * always hold the last node in either direction (ie down, right).
 */
HLNode *HLNode::last_sibling(void)
{
   HLNode *node = this;
   while (node && node->m_sibling)
      node = node->m_sibling;
   
   return node;
}

/**
 * @brief Convert the tag to a lower-case string.
 *
 * If the HLIndex::case_insensitive flag is set, we want to make lower-case the tags
 * for matching words to simplify case-insensitive matches easier later.  While we
 * could have converted the tag strings to lower case when originally saving them,
 * we don't want to disturb the rule lines, where the tag or the class names may be
 * mixed case words.
 */
void HLNode::tag_to_lower_case(void)
{
   if (m_tag)
   {
      char *p = m_tag;
      while (*p)
      {
         if (*p>=65 && *p<=90)
            *p += 32;

         ++p;
      }
   }
}

/**
 * @brief Build a new HLNode and add it as a child.
 *
 * This member function walks the sibling list of the first child
 * node to find the last child node.  The new node is added as a
 * sibling to the last child node.
 */
HLNode *HLNode::add_child(const char *name, const char *value)
{
   HLNode *n = new HLNode(name,value,this);
   if (m_child)
      m_child->last_sibling()->m_sibling = n;
   else
      m_child = n;

   return n;
}

/**
 * @brief Build a new HLNode and add to the end of the sibling list.
 *
 * This member function walks the sibling list of the node to find
 * the last sibling node.  The new node is added as a sibling the last
 * sibling node.
 */
HLNode *HLNode::add_sibling(const char *name, const char *value)
{
   HLNode *last = last_sibling();
   return last->m_sibling = new HLNode(name,value,last->m_parent);
}

HLNode *HLNode::seek_sibling(const char *tag)
{
   HLNode *node = this;
   while (node && *node!=tag)
      node = node->m_sibling;

   return node;
}

/**
 * @brief Returns a escape-resolved copy of a string.
 *
 * Creates a new string (using `new` operator) containing the escape-resolved
 * characters in the parameter @p str.  The string must be `delete`d to prevent
 * a memory leak.
 *
 * @param str String to be converted and copied.
 * @return Converted string.  The returned string must be `delete`d to
 *         prevent a memory leak
 *
 * @sa walk_str
 */
char *HLNode::save_str(const char *str)
{
   char *rval = nullptr;
   
   if (str)
   {
//@ [walk_str_count_characters]
      // Count the escape-resolved characters:
      int len = 0;
      // lambda function to serve as callback:
      auto fcount = [&len](int ch)
         {
            ++len;
         };
      // Call template function with the lambda function:
      walk_str(str, fcount);
//@ [walk_str_count_characters]

      // If any characters, allocate memory for the string
      // and fill it with the escaped-resolved characters.
      if (len>0)
      {
//@ [walk_str_copy_characters]
         // Size buffer using `len` counted in previous call to walk_str
         rval = new char[len+1];
         // Copy of pointer to walk the buffer:
         char *p = rval;
         // lambda function to serve as callback:
         auto fcopy = [&p](int ch)
            {
               *p++ = static_cast<char>(ch);
            };
         // Call template function with the lambda function:
         walk_str(str, fcopy);
         
         // terminate string:
         *p = '\0';
//@ [walk_str_copy_characters]
      }
   }
   return rval;
}

/** @brief Print the indent for priv_print. */
void HLNode::print_indent(FILE *f, int level)
{
   for (int i=0; i<level; ++i)
      fputs("   ", f);
}

/** @brief Internal print function with @p level parameter. */
void HLNode::priv_print(FILE *f, int level) const
{
   // Print current line:
   print_indent(f,level);
   if (m_tag)
      fprintf(f, "\"%s\"", m_tag);
   else
      fputs("/", f);
   
   if (m_value)
      fprintf(f, ": \"%s\"\n", m_value);
   else
      fputc('\n', f);

   // print children
   if (m_child)
      m_child->priv_print(f, level+1);
   // print siblings
   if (m_sibling)
      m_sibling->priv_print(f, level);
}


/**
 * @brief Constructor of an HLTree object.
 *
 * @param name Name of the tree, originally designed to be the language type
 *             that the tree contents represents.
 */

HLTree::HLTree(const char *name)
   : HLNode(nullptr, nullptr, nullptr), m_name(nullptr)
{
   size_t len = strlen(name);
   if (len)
   {
      char *buff = new char[len+1];
      memcpy(buff, name, len+1);
      m_name = buff;
   }
}

HLTree::~HLTree()
{
   delete [] m_name;
}


#ifndef EXCLUDE_TESTS
// Define EXCLUDE_TESTS for included source files:
#define EXCLUDE_TESTS

/** Call the save_str() function with the string in `str` and display the result. */
void save_a_string(const char *str)
{
   char *result = HLNode::save_str(str);
   if (result)
   {
      printf("\"%s\" -> \"%s\"\n", str, result);
      delete [] result;
   }
   else
      printf("\"%s\" was not translatable.\n", str);
}

/** Test a couple of strings. */
void test_save_str(void)
{
   save_a_string("My mama!");
   save_a_string("My \\#\\ \\\\Mama.");
}

/**
 * @brief Test of tree-building functions also serves as an example.
 *
 * @section Direct_HLNode_Methods Direct Methods
 * Use direct methods HLNode::direct_add_child and HLNode::direct_add_sibling
 * to add nodes.  When building a tree from a source document, the programmer
 * should add new nodes to the most recently acquired node pointer.
 * @snippet hlnode.cpp HLNode_direct_methods
 *
 * @section Indirect_HLNode_Methods Indirect Methods
 * The indirect methods always look for the last sibling of a node (either of the
 * child node or the sibling node), adding the new node at that point.  This is
 * less efficient when building a tree from scratch when the nodes come in an
 * orderly fashion.  This methods are useful will adding a new node to an existing
 * branch at a later time.
 * @snippet hlnode.cpp HLNode_indirect_methods
 */
void test_tree_building(void)
{
   HLTree tree("sql");
   HLNode *child, *entry;

   // Make a keyword branch:
//@ [HLNode_direct_methods]
   // Add a child node directly to the root node:
   child = tree.direct_add_child("keyword", "span.keyword");
   // Add first <i>keyword entry</i> child to the "keyword" node:
   entry = child->direct_add_child("if");
   // Add <i>keyword entry</i> siblings to the <i>keyword-entry</i> child.
   // Use the returned node and the direct_add_sibling method.
   entry = entry->direct_add_sibling("case");
   entry = entry->direct_add_sibling("\\ begin\\ ");
   entry = entry->direct_add_sibling("end");
//@ [HLNode_direct_methods]

//@ [HLNode_indirect_methods]   
   // Make a comment branch:
   HLNode *comments = tree.add_child("comment", "span.comment");
   // Add two children to the branch.  The second will be added as a
   // sibling of the first.
   comments->add_child("start", "/*");
   comments->add_child("end", "*/");
//@ [HLNode_indirect_methods]

   // Add to existing branch:
   HLNode *keywords = tree.seek_child("keyword");
   if (keywords)
   {
      keywords->add_child("loop");
      keywords->add_child("while");
   }


   printf("About to print the tree.\n");
   tree.print(stdout);
   
}

int main(int argc, char **argv)
{
   test_save_str();
   test_tree_building();
}

#endif
