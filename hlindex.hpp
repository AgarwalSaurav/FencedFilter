// -*- compile-command: "g++ -std=c++11 -Wall -Werror -Weffc++ -pedantic -ggdb -o hlindex hlindex.cpp"  -*-

#ifndef HLINDEX_HPP
#define HLINDEX_HPP

#include <stdio.h>
#include "hlnode.hpp"




/**
 * @brief Load and access highlight files.
 */
class HLParser
{
public:
   HLParser(FILE *f, HLNode *root);
   ~HLParser() { }

   inline bool hyphenated_tags(void) const  { return m_hyphenated_tags; }
   inline bool case_insensitive(void) const { return m_case_insensitive; }

private:
   static char s_read_buff[];
   static const size_t s_len_buff;
   
   FILE *m_file;

   int  m_cur_level;  /**< Number of leading white space characters of the most recently-read line. */
   char *m_cur_tag;   /**< Tag name of most recently-read line. */
   char *m_cur_value; /**< Value of the most recently-read line. */

   bool m_hyphenated_tags;  /**< HL-file flag to match words with hyphens.
                             *   Normally, a hyphen is interpreted as a subtraction
                             *   operator and would thus mark the end of a word.
                             *
                             *   This flag does not affect HL file parsing, the
                             *   hyphens in a tag will always be preserved,  During
                             *   processing, however, without this flag, word matches
                             *   will be inconsistent.
                             */
   bool m_case_insensitive; /**< HL-file flag to perform case-insensitive keyword
                             *   searches.  SQL is a case-insensitive language, where
                             *   @e SELECT, @e Select, and @e select are considered
                             *   to be the same value.
                             *
                             *   This flag affects both parsing the HL file (converting
                             *   all tags to lower-case), and while scanning the
                             *   fenced code.
                             */

   // Parsing functions:
private:
   inline int level(void) const         { return m_cur_level; }
   inline const char *tag(void) const   { return m_cur_tag; }
   inline const char *value(void) const { return m_cur_value; }

   /** @brief Indicates that more file data is available. */
   bool file_ready(void) const { return m_file!=nullptr; }

   bool cur_line_is_node(void) const { return m_cur_tag!=nullptr; }

   void print_line(int level, const char *name, const char *value);
   
   bool set_flag_from_line(const char *str);
   void process_line(void);
   bool read_line(void);

   void do_node(HLNode *host, int level);

   

   // Delete errc++ requested operators
   HLParser(const HLParser &)             = delete;
   HLParser & operator=(const HLParser &) = delete;
};


/**
 * @brief Builds an index from the entries of a highlight file.
 *
 * The class is implemented as a singly-linked list.  Requests for processing
 * specific file types will search the linked indexes for a match, returning
 * the match if found, otherwise returning nullptr.
 */
class HLIndex
{
public:
   static const HLIndex* get_index(const char *type);

//   inline int count(void) const            { return m_count; }
   inline int is_empty(void) const         { return m_entries==nullptr && m_comments==nullptr; } 
   void print(FILE *f) const;

   const HLNode *seek(const char *tag) const;
   const HLNode *seek_word(const char *str) const;
   const HLNode *seek_comment(const char *str) const;

   static int str_match_sensitive(const char *haystack,
                                  const char *needle,
                                  bool is_tag);
   
   static int str_match_insensitive(const char *haystack,
                                    const char *needle,
                                    bool is_tag);

   static bool simple_name_allow(int ch);
   static bool hyphenated_name_allow(int ch);

   /**
    * @brief Returns true if character allowed in a name.
    *
    * The primary purpose of this function is to detect the limits of a name.  That is,
    * to decide when a potential name starts and ends.  A word detected as a string
    * of these characters is submitted to an HLIndex object to decide if and how it
    * should be highlighted.
    *
    * It is easier and more efficient to compare a character to this function's three
    * ranges plus one character than it would be to detect white-space + punctuation +
    * mathematical and logical operators.
    *
    * @param ch Character to consider
    * @returns TRUE if @p ch is an '_' (underscore) or in one of the ranges A-Z, a-z,
    *          or 0-9; FALSE otherwise.
    */
   static inline bool allowed_in_name(int ch)
   {
      return (*s_word_eligible_char_func)(ch);
   }

   
   
private:
   HLIndex(HLNode *root,
           bool hyphenated_tags=false,
           bool case_insensitive=false);
   ~HLIndex();

   static FILE *find_and_open_file(const char *type);
   
   inline bool is_equal(const char *tag) const { return m_root && m_root->is_equal(tag); }
   static const HLIndex* seek_index(const char *type);
   static HLIndex *get_last(void);

public:   
   /** Enables switching between case-sensitive and case-insensitive comparisons. */
   typedef int (*Str_Match_Func)(const char*, const char*, bool is_tag);

   static Str_Match_Func get_str_match_func(bool case_sensitive=true)
   {
      return case_sensitive ? str_match_sensitive : str_match_insensitive;
   }

   /** Enabled switching between allowing and not allowing hyphens in names. */
   typedef bool (*Word_Eligible_Char_Func)(int ch);

   static Word_Eligible_Char_Func get_name_char_checker(void)
   {
      return s_word_eligible_char_func;
   }
   static bool hyphenated_names_allowed(void)
   {
      return s_word_eligible_char_func == hyphenated_name_allow;
   }
   static void set_hyphenated_names_allowed(bool allow_hyphens = true)
   {
      s_word_eligible_char_func = allow_hyphens ? hyphenated_name_allow : simple_name_allow;
   }
   static void set_name_char_checker(Word_Eligible_Char_Func f)
   {
      s_word_eligible_char_func = f;
   }

private:
   
   static HLIndex s_base;   /**< Base object from which searches will start
                             *   and the destructor will call upon termination
                             *   of the application.
                             */

   /** Function pointer to hyphens-allowed, -not-allowed char comparison function. */
   static Word_Eligible_Char_Func s_word_eligible_char_func;
   
   HLIndex        *m_next;  /**< Pointer to next index.  Not constant because
                             *   its @p m_index may be changed when a new index
                             *   is added.
                             */
   
   HLNode   *m_root;        /**< The root HLNode of this file type.  */
   
   HLNode** m_entries;      /**< Array of pointers to nodes of m_root. */
   HLNode** m_last_entry;   /**< Used to test out-of-bounds when incrementing. */

   HLNode** m_comments;
   HLNode** m_last_comment; /**< Used to test out-of-counts when incrementing. */

   /**
    * @defgroup HLIndex_Processing_Flags
    *
    * These member variables are flags to contradict the default
    * processing mode.
    *
    * By default, hyphens are not allowed in tags, and string comparisons are
    * case-sensitive.
    *
    * These flags can be set to change default behavior with !-prefixed lines in
    * the highlighting file.
    * @{
    */
   bool m_hyphenated_tags;
   bool m_case_insensitive;
   /** @} */

   Str_Match_Func m_str_match_func;  /**< Function pointer for either case-sensitive
                                      *   or case-insensitive versions of a string
                                      *   comparison function.
                                      */
   inline int str_match(const char *haystack, const char *needle, bool is_tag) const
   { return (*m_str_match_func)(haystack,needle,is_tag); }

   int source_count(void);
   void source_scan(void);
   
   template <class Func>
   void walk_tags(Func f)
   {
      HLNode *branch = m_root->first_child();
      HLNode *node;
      while (branch)
      {
         node = branch->first_child();
         while (node)
         {
            f(node);
            node = node->next_sibling();
         }
         
         branch = branch->next_sibling();
      }
   }
   
   // Delete effc++ requested operators
   HLIndex(const HLIndex &)             = delete;
   HLIndex & operator=(const HLIndex &) = delete;
};



#endif
