// -*- compile-command: "g++ -std=c++11 -Wall -Werror -Weffc++ -pedantic -ggdb -o hlindex hlindex.cpp"  -*-

/** @file */

#include "hlindex.hpp"
#include <ctype.h>   // for isspace()
#include <string.h>  // for strlen()
#include <alloca.h>  // for alloca()

#include <stdlib.h>  // for qsort()


/*
 * Beginning of HLParser class implementations.
 */
char HLParser::s_read_buff[1024];
const size_t HLParser::s_len_buff = sizeof(HLParser::s_read_buff);

HLParser::HLParser(FILE *f, HLNode *root)
   : m_file(f), m_cur_level(0), m_cur_tag(nullptr), m_cur_value(nullptr),
     m_hyphenated_tags(false), m_case_insensitive(false)
{
   do_node(root, -1);
}

void HLParser::print_line(int level, const char *tag, const char *value)
{
   printf("at level %02d: \"%20s\"", level, tag);
   if (value)
      printf(" = \"%s\"\n", value);
   else
      fputc('\n', stdout);
}

/**
 * @brief Called by process_line when encountering possible flag direction.
 *
 * @param str Pointer to s_read_buff to an unescaped '!' that may be a flag.
 * @return TRUE if flag matched, FALSE otherwise.
 *
 * This function compares the string for !ht, !hyphen (first part of
 * _hyphenated-tags_), !ci, or case-i (the first part of _case-insensitive_),
 * setting HLParser flags as appropriate.
 */
bool HLParser::set_flag_from_line(const char *str)
{
   // Skip initial '!' if found, return FALSE indicating no flag set if '!' not found.
   if (*str=='!')
      ++str;
   else
      return false;
   
   bool flag_set = false;
   
   char p1 = *str;
   char p2 = *(str+1);
   char p3 = *(str+2);

   if (p3=='\0')
   {
      if (p1=='h' && p2=='t')
         flag_set = m_hyphenated_tags = true;
      else if (p1=='c' && p2=='i')
         flag_set = m_case_insensitive = true;
   }
   else if (strncmp("hyphen", str, 6)==0)
      flag_set = m_hyphenated_tags = true;
   else if (strncmp("case-i", str, 6)==0)
      flag_set = m_case_insensitive = true;

   return flag_set;
}

/**
 * @brief Parse static char buffer s_read_buff for node information.
 *
 * This function sets the member variables m_cur_level, m_cur_tag, and
 * m_cur_value according to the values found in s_read_buff.
 */
void HLParser::process_line(void)
{
   // pointers for iterating over the buffer:
   char *p = nullptr;
   
   // convert line-ending newline to '\0' rather than
   // check for it with each iteration:
   p = &s_read_buff[strlen(s_read_buff)-1];
   if (*p=='\n')
      *p = '\0';

   // Set pointer to start processing
   p = s_read_buff;
   
   // values of the current line:
   m_cur_level = 0;
   m_cur_tag = m_cur_value = nullptr;

   // flags to indicate progress:
   bool stop_comment = false;
   bool found_colon = false;

   // loop to count spaces as indication of level
   while (*p && isspace(*p))
   {
      ++m_cur_level;
      ++p;
   }

   if (*p)
      m_cur_tag = p;

   // Continue loop after level found to get line contents:
   while (*p)
   {
      // If we haven't found the colon yet, allow continued incrementing
      // until the colon is found.
      if (!found_colon)
      {
         // A backslash indicates that we need to keep the backslash and
         // the following character, ignoring any action that would otherwise
         // be taken on its behalf, ie a whitespace, colon, or '#' comment
         // marker.  "continue" to check the next character.
         if (*p=='\\')
         {
            p += 2;
            continue;
         }

         // To be a flag directive, the '!' must be at column 0
         if (*p=='!' && p==s_read_buff)
         {
            // Try to set a flag, skip the rest of the line if flag is set:
            if (set_flag_from_line(p))
               return;
         }

         // The tag is terminated if the character is a whitespace, colon, or #.
         // Change to '\0' to terminate the tag, the fall-out of conditional to
         // the ++p to continue looking for colon (if not yet found), the beginning
         // of a value (if the colon has been found), or the end of the string.
         
         if (*p==':')
         {
            found_colon = true;

            // Terminate m_tag string
            *p++ = '\0';
            
            // Skip to possible beginning of m_value, if any:
            while (*p && isspace(*p))
               ++p;

            // Since we're already where we need to be,
            // preempt loop-ending ++p:
            continue;
         }
         else if (isspace(*p))
            *p = '\0';
         else if (*p=='#')
         {
            *p = '\0';
            stop_comment = true;
            break; 
         }
      }
      // Save the first character (as left behind by !found_colon && *p==':' above):
      else if (!m_cur_value)
         m_cur_value = p;

      // increment the character
      ++p;
   }
   
   // Deal with comment-initiated early termination:
   if (stop_comment)
   {
      // walk back any unescaped vspaces, stopping for '\0' if already found
      while (*p && isspace(*--p)  && *(p-1)!='\\')
         ;

      // If not backed against the start of the buffer, simply
      // terminate whatever string we're working on:
      if (*p && p > s_read_buff)
         *(++p) = '\0';
   }
}

bool HLParser::read_line(void)
{
   char *p = fgets(s_read_buff, s_len_buff, m_file);
   if (p)
   {
      process_line();
      return true;
   }
   else
   {
      m_file = nullptr;
      return false;
   }
}

void HLParser::do_node(HLNode *host, int entry_level)
{
   // Get next saveable line:
   while (read_line() && !cur_line_is_node())
      ;

   while (file_ready())
   {
      if (cur_line_is_node())
      {
         int diff = level() - entry_level;

         
         // If at level() is higher than @p level, the current line
         // is a child of @p host.  Add a node as a child, and pass the
         // child as a host to a recursive call to do_node().  do_node()
         // will return when level() <= @p level, meaning the pending
         // node is either a sibling or ancestor.
         if (diff > 0)
         {
            do_node(host->direct_add_child(tag(), value()), level());

            // Return to top of loop without a new read_line() in order
            // to determine where to put the pending node:
            continue;
         }

         // If lower level, return to process the pending ancestor node:
         else if (diff < 0)
            break;
         
         // If at same level, add node as a sibling, then
         // fall through to read next line:
         else //  if (diff==0)
            host = host->direct_add_sibling(tag(), value());
      }

      read_line();
   }
}

/*
 * Beginning of HLIndex class functions:
 */

HLIndex HLIndex::s_base(nullptr);
HLIndex::Word_Eligible_Char_Func HLIndex::s_word_eligible_char_func = HLIndex::hyphenated_name_allow;

/**
 * @brief This is the public way to access HLIndex instances.
 *
 * This function first searches for an existing HIndex with the same type.
 * If an existing index is not found, it will try to open and parse a
 * highlighting file with the type name followed by `.hl`.
 *
 * If the highlighting file has been found, this function will return the
 * HLIndex of the file.  Otherwise, nullptr will be returned.
 *
 * Internally, the first request for a highlighting file will always
 * create a HLIndex branch, but for unavailable extensions, it will
 * be empty.  This will prevent FencedFilter from repeated attempts to
 * find a missing file.  When HLIndex finds an empty index, it will
 * return nullptr to revert to default doxygen processing.
 *
 * @param type Typical extension of the file type
 * @return A pointer to an HLIndex if found, nullptr otherwise.
 */
const HLIndex* HLIndex::get_index(const char *type)
{
   const HLIndex *rval = seek_index(type);
   if (!rval)
   {
      // Make an HLNode and attempt to populate it
      // with the contents of a highlight file:
      HLNode *root = new HLNode(type);
      FILE *f = find_and_open_file(type);
      if (f)
      {
         HLParser hlp(f, root);
         // Regardless of highlight file success, add a new
         // HLIndex to the chain with the new HLNode:
         HLIndex *last = get_last();
         rval = last->m_next = new HLIndex(root,
                                           hlp.hyphenated_tags(),
                                           hlp.case_insensitive());
      }
   }

   if (rval && !rval->is_empty())
      return rval;
   else
      return nullptr;
}

bool HLIndex::simple_name_allow(int ch)
{
   return (ch>=48 && ch<=57)    // allow numerals,
      || (ch>=65 && ch<=90)     // allow upper-case letters,
      || (ch>=97 && ch<=122)    // allow lower-case letters,
      || ch==95;                // allow underscore
}

bool HLIndex::hyphenated_name_allow(int ch)
{
   return (ch>=48 && ch<=57)    // allow numerals,
      || (ch>=65 && ch<=90)     // allow upper-case letters,
      || (ch>=97 && ch<=122)    // allow lower-case letters,
      || ch==95                 // allow underscore
      || ch==45;                // allow hyphen,
}

/**
 * @brief Compares the the comparison results to see if matched
 *
 * @param sub_haystack Pointer to last comparison made in haystack.
 * @param sub_needle Pointer to last comparison made in needle.
 * @param is_tag Compares to end of word in sub_haystack,
 *        otherwise just last character.
 * @return Whether good match made or not.
 */
bool full_str_match_made(const char *sub_haystack,
                         const char *sub_needle,
                         bool is_tag)
{
   // If completely matched the needle
   if (*sub_needle=='\0')
   {
      // If haystack also fully consumed,
      // it matches regardless of is_word value:
      if (*sub_haystack=='\0')
         return true;

      if (!is_tag || !HLIndex::allowed_in_name(*sub_haystack))
         return true;
   }
   return false;
}

/**
 * @brief Returns the number of matching characters between @p haystack and @p needle.
 *
 * This function compares the beginning of @p haystack with the characters in @p needle
 * up to the '\0' terminator of @p needle.
 *
 * @param haystack String where a string is searched.
 * @param needle NULL-terminated string to compare against the start of @p haystack.
 * @param is_tag Comparing strings as a tag, which looks for a terminating non-word char.
 * @return Number of matching characters.
 */
int HLIndex::str_match_sensitive(const char *haystack,
                                 const char *needle,
                                 bool is_tag)
{
   // This function shouldn't be called with missing or empty strings.
   // If this function causes an exception, turn on the following
   // assertions to determine where the mistake was made:
   //
   // assert(haystack && *haystack);
   // assert(needle && *needle);
   
   // Skip everything if the first two characters don't match:
   if (*haystack==*needle)
   {
      const char *save_needle = needle;
      while (*needle && *needle==*haystack)
      {
         ++needle;
         ++haystack;
      }

      if (full_str_match_made(haystack, needle, is_tag))
         return needle - save_needle;

      return 0;
   }

   return 0;
}

inline char toLowerCase(char c) { return (c>=65 && c<=90) ? c+32 : c; }

/**
 * @brief Returns the number of matching characters between @p haystack and @p needle.
 *
 * This function compares the beginning of @p haystack with the characters in @p needle
 * up to the '\0' terminator of @p needle.
 *
 * The comparison is made while converting characters in @p haystack to lower case.
 * In our usage, if case-insensitive comparisons are called for, the @p needle,
 * which is a tag from a highlighting file, will have already been converted to
 * lower case when the hl file was read.
 *
 * @param haystack String where a string is searched.
 * @param needle NULL-terminated string to compare against the start of @p haystack.
 * @param is_tag Comparing strings as a tag, which looks for a terminating non-word char.
 * @return Number of matching characters.
 */
int HLIndex::str_match_insensitive(const char *haystack,
                                   const char *needle,
                                   bool is_tag)
{
   // This function shouldn't be called with missing or empty strings.
   // If this function causes an exception, turn on the following
   // assertions to determine where the mistake was made:
   //
   // assert(haystack && *haystack);
   // assert(needle && *needle);
   
   // Skip everything if the first two characters don't match:
   if (toLowerCase(*haystack)==*needle)
   {
      const char *save_needle = needle;
      
      while (*needle && *needle==toLowerCase(*haystack))
      {
         ++needle;
         ++haystack;
      }

      if (full_str_match_made(haystack, needle, is_tag))
         return needle - save_needle;
   }

   return 0;
}


/**
 * @brief Returns, if found, the node whose tag matches @p tag.
 *
 * @param tag NULL-terminated string of a word for which to search.
 * @return The matching HLNode* if found, NULL otherwise.
 *
 * @todo OPTIMIZE: since the tag list is sorted alphabetically, this function should,
 * at the very least, stop searching once the @p tag is greater alphabetically that
 * the currently selected HLNode*.  For long lists, it might make sense to search from
 * the middle and proceeding away from the middle and terminating the search
 * according to the direction of the search.
 */
const HLNode* HLIndex::seek(const char *tag) const
{
   HLNode **n = m_entries;
   while (n<m_last_entry)
   {
      if ((*n)->is_equal(tag))
         return const_cast<const HLNode*>(*n);
      ++n;
   }

   return nullptr;
}

const HLNode* HLIndex::seek_word(const char *str) const
{
   HLNode **n = m_entries;
   int len;
   
   while (n < m_last_entry)
   {
      const char *c = (*n)->tag();

      if (strncmp(str, "background-attachment", 21)==0)
         if ((*n)->is_equal("background"))
            len = 1;
      
      if ((len=str_match(str, c, true)))
         return const_cast<const HLNode*>(*n);
      else
         ++n;
   }

   return nullptr;
}

/**
 * @brief Returns, if found, the node whose comment tag matches the beginning of @p str.
 *
 * @param str String that may be the start of a comment.
 * @return The matching HLNode* if found, NULL otherwise.
 */
const HLNode *HLIndex::seek_comment(const char *str) const
{
   HLNode **n = m_comments;
   int len;
   while (n < m_last_comment)
   {
      const char *c = (*n)->tag();
      if ((len=str_match(str, c, false)))
         return const_cast<const HLNode*>(*n);
      else
         ++n;
   }

   return nullptr;
}

void HLIndex::print(FILE *f) const
{
   fputc('\n', stdout);
   
   if (m_hyphenated_tags)
      printf("Processing with hyphenated tags.\n");

   if (m_case_insensitive)
      printf("Processing case-insensitive tags.\n");
   
   HLNode **n;
   if (m_entries)
   {
      printf("\nListing tags:\n");
      n = m_entries;
      while (n<m_last_entry)
      {
         printf("\"%s\"\n", (*n)->tag());
         ++n;
      }
   }
   if (m_comments)
   {
      printf("\nListing comments:\n");
      n = m_comments;
      while (n<m_last_comment)
      {
         printf("\"%s\"\n", (*n)->tag());
         ++n;
      }
   }
}



HLIndex::HLIndex(HLNode *root, bool hyphenated_tags, bool case_insensitive)
   : m_next(nullptr), m_root(root),
     m_entries(nullptr), m_last_entry(nullptr),
     m_comments(nullptr), m_last_comment(nullptr),
     m_hyphenated_tags(hyphenated_tags),
     m_case_insensitive(case_insensitive),
     m_str_match_func(case_insensitive?str_match_insensitive:str_match_sensitive)
     
{
   set_hyphenated_names_allowed(hyphenated_tags);
   
   if (root)
      source_scan();
}

/** Destructor also sets allowed_in_name to non-hyphenated version. */
HLIndex::~HLIndex()
{
   s_word_eligible_char_func = simple_name_allow;
   
   delete m_next;
   delete m_root;
   delete [] m_entries;
}

/**
 * @brief Looks for and opens an appropriate highlight file.
 *
 * @param type The type string that opens a fenced code area in doxygen.
 * @return An open file pointer or NULL if an appropriate file was not found.
 *
 * For now, this function simply searches the current directory for a
 * file whose name matches @p type, followed by `.hl`.  A possible future
 * enhancement might be to look in an `.conf` file for alternate search
 * paths.
 */
FILE *HLIndex::find_and_open_file(const char *type)
{
   // Use stack memory to create file name with .hl extension:
   size_t len = strlen(type);
   
   // Buffer with 4 extra chars, '.hl' + '\0':
   char *filepath = static_cast<char*>(alloca(len+4));
   char *p = filepath;
   memcpy(p, type, len);
   p += len;
   memcpy(p, ".hl", 3);
   p += 3;
   *p = '\0';

   return fopen(filepath, "r");
}


/** @brief Walks HLIndex linked list to find one that supports @p type. */
const HLIndex *HLIndex::seek_index(const char *type)
{
   const HLIndex *cindex = &s_base;
   while (cindex)
   {
      if (cindex->is_equal(type))
         return cindex;

      cindex = cindex->m_next;
   }

   return nullptr;
}

/** @brief Get last of HLIndex linked list as host for new HLIndex. */
HLIndex *HLIndex::get_last(void)
{
   HLIndex *rval = &s_base;
   while (rval->m_next)
      rval = rval->m_next;

   return rval;
}

int hlnode_sorter(const void *lh, const void *rh)
{
   const HLNode *lhn = *static_cast<const HLNode**>(const_cast<void*>(lh));
   const HLNode *rhn = *static_cast<const HLNode**>(const_cast<void*>(rh));
   return strcmp(lhn->tag(), rhn->tag());
}

/**
 * @brief Scans the HLNode root pointer for recognized tags.
 */
void HLIndex::source_scan(void)
{
   int count_words = 0;
   int count_comments = 0;
   auto fcount = [&count_words, &count_comments, this](HLNode *node)
   {
      const char *tag = node->tag();
      if (allowed_in_name(*tag))
         ++count_words;
      else if (!isspace(*tag))
         ++count_comments;
   };

   walk_tags(fcount);

   if (count_words + count_comments)
   {
      // Prepare the entries (words) list:
      HLNode **arr_words = nullptr;
      if (count_words)
      {
         arr_words = new HLNode*[count_words];
         memset(arr_words,0,count_words*sizeof(HLNode*));
         m_entries = arr_words;
         m_last_entry = m_entries + count_words;
      }

      // Conditionally prepare the comments list:
      HLNode **arr_comments = nullptr;
      if (count_comments)
      {
         arr_comments = new HLNode*[count_comments];
         memset(arr_comments,0,count_comments*sizeof(HLNode*));
         m_comments = arr_comments;
         m_last_comment = m_comments + count_comments;
      }

      // Lambda function to save words and comments to respective arrays:
      auto fsave = [&arr_words, &arr_comments, this](HLNode *node)
         {
            if (m_case_insensitive)
               node->tag_to_lower_case();
            
            const char *tag = node->tag();
            if (allowed_in_name(*tag))
            {
               *arr_words = node;
               ++arr_words;
            }
            else if (!isspace(*tag))
            {
               *arr_comments = node;
               ++arr_comments;
            }
         };

      walk_tags(fsave);

      // Sort lists that exist:
      if (count_words)
         qsort(m_entries, count_words, sizeof(HLNode*), hlnode_sorter);
      if (count_comments)
         qsort(m_comments, count_comments, sizeof(HLNode*), hlnode_sorter);
   }
}






#ifndef EXCLUDE_TESTS
#define EXCLUDE_TESTS

#include "hlnode.cpp"

/**
 * @brief Test opening highlighting file.
 */

void check_tag(const HLIndex *ndx, const char *tag)
{
   const HLNode *el = ndx->seek(tag);
   if (el)
   {
      const char *val = nullptr;
      const HLNode *par = el->parent();
      if (par->has_value())
         val = par->value();
      else
         val = par->tag();
               
      printf("Found \"%s\", surrounding with %s.\n", tag, val);
   }
}

int comp_words(const char *haystack,
               const char *needle,
               bool case_sensitive = true,
               bool allow_hyphens = false)
{
   int matched = 0;

   auto strmatch = HLIndex::get_str_match_func(case_sensitive);
   
   auto saved_checker = HLIndex::get_name_char_checker();
   HLIndex::set_hyphenated_names_allowed(allow_hyphens);

   matched = (*strmatch)(haystack, needle, true);
   
   HLIndex::set_name_char_checker(saved_checker);

   return matched;
}

void test_str_match_word(void)
{
   int count;
   char hay[] = "John Doerr background-color : #FFFFFF; color : #FFFFFF;";
   const char *needle;

   printf("\nBeginning test_str_match_word:\n");

   needle = "john doerr";
   
   count = comp_words(hay, needle, false, false);
   printf("\"%s\" from \"%s\" = %d (insensitive, no hyphens)\n", hay, needle, count);
   count = comp_words(hay, needle, true, false);
   printf("\"%s\" from \"%s\" = %d (sensitive, no hyphens)\n", hay, needle, count);

   count = comp_words(hay, needle, true, true);
   printf("\"%s\" from \"%s\" = %d (sensitive, hyphens)\n", hay, needle, count);
   count = comp_words(hay, needle, false, true);
   printf("\"%s\" from \"%s\" = %d (insensitive, hyphens)\n", hay, needle, count);

   
   
   printf("\n\n");
}

void test_str_match_comment(void)
{
   char hay[] = "<!-- This is a block comment -->" ;

   int count;
   const char *needle = "<!-- ";
   count = HLIndex::str_match_sensitive(hay, needle, false);
   printf("Match should succeed comparing %s with %s,\nReturns %d, full match.\n",
          needle, hay, count);

   needle = "<!- ";
   count = HLIndex::str_match_sensitive(hay, needle, false);
   printf("Match should fail comparing %s with %s,\nReturns %d, incomplete match.\n",
          needle, hay, count);

   needle = " <!-- ";
   count = HLIndex::str_match_sensitive(hay, needle, false);
   printf("Match should fail comparing %s with %s,\nReturns %d, bad first char.\n",
          needle, hay, count);
}


int main(int argc, char **argv)
{
   if (argc==1)
   {
      test_str_match_word();
      test_str_match_comment();
      
      printf("\nUsage: hlindex filetype [,filetype, ...]\n");
      return 1;
   }
   else
   {
      for (int i=1; i<argc; ++i)
      {
         const char *tag = argv[i];
         const HLIndex* ndx = HLIndex::get_index(tag);
         if (ndx)
            ndx->print(stdout);
         else
            printf("We failed to read type %s.\n", tag);
            
      }


      // const HLIndex *ndx = HLIndex::get_index("sql");
      // if (ndx)
      // {
      //    ndx->print(stdout);
         
      //    check_tag(ndx, "#");
      //    check_tag(ndx, "begin");
      // }
   }
}


#endif
