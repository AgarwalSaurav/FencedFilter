// -*- compile-command: "g++ -std=c++11 -Wall -Werror -Weffc++ -pedantic -ggdb -o hlnode hlnode.cpp"  -*-

/** @file */

#ifndef HLNODE_HPP
#define HLNODE_HPP

#include <string.h>

/**
 * @brief Walks through a string, one escape-resolved char at a time.
 *
 * To copy a string with escaped characters resolved, it is necessary
 * to use the same algorithm to count (to allocate a sufficiently
 * large buffer), and copy characters.  Using this template function
 * allows the two operations to use the same code.
 *
 * It works with a lambda function that is called for each character.
 *
 * Escaped characters are those preceded by a backslash ('\').  The
 * callback function will be called for the character following the
 * backslash, but not the backslash itself.
 *
 * @tparam F  A function that takes a single int parameter,
              representing a single character.
 * @param str String to be walked.
 * @param F   An instance of the template function.
 *
 * Consult the code for examples of how to use walk_str().  walk_str() is used
 * to count characters and again to copy the characters to a buffer sized
 * according to the count of characters.
 *
 * @section walk_str Examples
 * First count the characters to determine byte count needed to hold string.
 * @snippet hlnode.cpp walk_str_count_characters
 *
 * Having counted the characters, copy the buffer.
 * @snippet hlnode.cpp walk_str_copy_characters
 */
template <class F>
void walk_str(const char *str, F f)
{
   const char *p = str;
   while (*p)
   {
      if (*p=='\\')
      {
         const char *t = strchr("nrt", *++p);
         if (t)
            f('\\');
         
         f(*p);
      }
      else
         f(*p);

      ++p;
   }
}


/**
 * @brief Class to hold one line of the Highlight file, with pointers to relatives.
 *
 * @sa test_tree_building
 */
class HLNode
{
public:
   HLNode(const char *tag=nullptr, const char *value=nullptr, HLNode *parent=nullptr);
   virtual ~HLNode();

   const bool has_value(void) const { return m_value!=nullptr; }
   const char *tag(void) const      { return m_tag; }
   const char *value(void) const    { return m_value; }

   HLNode *last_sibling(void);

   void tag_to_lower_case(void);

   inline const HLNode *parent(void) const        { return m_parent; }
   
   inline const HLNode *next_sibling(void) const  { return m_sibling; }
   inline const HLNode *last_sibling(void) const  { return const_cast<const HLNode*>(last_sibling()); }
   inline const HLNode *first_child(void) const   { return m_child; }
   inline const HLNode *last_child(void) const    { return m_child ? m_child->last_sibling() : nullptr; }

   inline HLNode *next_sibling(void) { return m_sibling; }
   inline HLNode *first_child(void)  { return m_child; }
   inline HLNode *last_child(void)   { return m_child ? m_child->last_sibling() : nullptr; }

   
   HLNode *add_child(const char *tag, const char *value=nullptr);
   HLNode *add_sibling(const char *tag, const char *value=nullptr);

   /** @brief Installs a new node at m_child, discarding previous value.  Make leak memory. */
   HLNode *direct_add_child(const char *tag, const char *value=nullptr)   { return m_child = new HLNode(tag,value,this); }
   /** @brief Installs a new node at m_sibling, discarding previous value.  Make leak memory. */
   HLNode *direct_add_sibling(const char *tag, const char *value=nullptr) { return m_sibling = new HLNode(tag,value,m_parent); }

   void print(FILE *out) const  { priv_print(out, 0); }

   // Comparison and seeking functions
   bool is_equal(const char *tag) const  { return !strcmp(tag, m_tag); }
   bool operator==(const char *tag) const { return !strcmp(tag, m_tag); }
   bool operator!=(const char *tag) const { return !!strcmp(tag, m_tag); }
   
   HLNode *seek_sibling(const char *tag);
   inline const HLNode *seek_sibling(const char *tag) const { return const_cast<const HLNode*>(seek_sibling(tag)); }
   inline       HLNode *seek_child(const char *tag)         { return m_child ? m_child->seek_sibling(tag) : nullptr; }
   inline const HLNode *seek_child(const char *tag) const   { return const_cast<const HLNode*>(seek_child(tag)); }

   static char * save_str(const char *str);

private:
   HLNode *m_parent;
   HLNode *m_child;
   HLNode *m_sibling;
   char   *m_tag;    /**< This private variable is @e not const for flexibility. */
   char   *m_value;  /**< This private variable is @e not const for flexibility. */

   static void print_indent(FILE *f, int level);
   void priv_print(FILE *fout, int level=0) const;

   bool operator<(const HLNode &rh) const { return strcmp(m_tag,rh.m_tag)<0; }

private:
   // Delete effc++ requested operators
   HLNode(const HLNode &)             = delete;
   HLNode & operator=(const HLNode &) = delete;
};

 /**
 * @brief An unnamed Class that names and holds the root of a network of HLNodes.
 *
 * Actually, the HLTree @b is the root of the tree.  Use the HLNode interface
 * of this and its descendents to build the tree.
 *
 * @sa test_tree_building
 */
class HLTree : public HLNode
{
public:
   HLTree(const char *name);
   virtual ~HLTree();

   // Delete effc++ requested operators
   HLTree(const HLTree &)             = delete;
   HLTree & operator=(const HLTree &) = delete;
   
private:
   const char *m_name;

};

#endif

