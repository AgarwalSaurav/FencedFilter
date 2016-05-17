/** @file */

/**
 * @mainpage FencedFilter
 * 
 * This program will search a document for fenced code according to the rules
 * found at [CommonMark](http://spec.commonmark.org/0.25/#fenced-code-blocks).
 *
 * Other code will be left alone, but keywords and comments can be highlighted
 * within the fenced area according to the info string (an optional string
 * following the opening code fence).
 *
 * The program will search for a file whose name is the same as the info string
 * with an extension of .hl (for highlight).  Words in the fenced code block will
 * be enclosed with `div`s or `span`s with class set according to matching
 * words in the highlight file.
 *
 * - @ref userguide.md
 * - @ref fencedfilter.cpp
 * - @ref IDENTIFYING_COMMENTS
 * - @ref TestBlockComments
 */

#include <stdio.h>
#include <string.h>  // for strlen
#include <ctype.h>   // for isspace
#include <stdint.h>  // for uint16_t
#include <alloca.h>  // for alloca
#include <assert.h>

#include "hlindex.hpp"

#define FF_VERSION_MAJOR 0
#define FF_VERSION_MINOR 1


// Function prototypes as necessary.
void process_line(char *str);
void process_code_line(char *str);
void process_block_comment_line(char *str);
void process_doxy_block_comment_line(char *str);

/**
  * @page TestBlockComments Test Block Comments
  *
  * Consider the following fenced code blocks:
  *
  * - SQL language fenced code:
  * ~~~.sql
    -- This is an SQL comment
    CREATE PROCEDURE Bogus(id INT UNSIGNED)
    BEGIN
       SELECT DATE();
    END $$
  * ~~~
  *
  * - C language fenced code:
  * ~~~{.c}
  * // This is a comment, how's it look?
  * int main(int argc, char** argv)
  * {
  *    return 0;
  * }
  * ~~~
  *
  * - Embedded HTML
 <div class="fragment"><div class="line">-- This is a line comment for SQL</div>
 <div class="line"><span class="keywordflow">BEGIN</span></div>
 <div class="line">   do</div>
 <div class="line">   <span class="keywordflow">while</span>();</div>
 <div class="line"><span class="keywordflow">END</span></div>
 </div>
  */

/*
<div class="fragment"><div class="line">-- This is a line comment for SQL</div>
<div class="line"><span class="keywordflow">BEGIN</span></div>
<div class="line">   do</div>
<div class="line">   <span class="keywordflow">while</span>();</div>
<div class="line"><span class="keywordflow">END</span></div>
</div>
  */

enum STATE
{
   S_CODE,
   S_LINE_COMMENT,
   S_BLOCK_COMMENT,
   S_DOXY_BLOCK_COMMENT,
   S_FENCED
};

char scan_buff[1024];       /**< Global scan buffer to allow access for debugging. */

STATE state = S_CODE;       /**< State variable to track current processing mode. */
STATE fence_return_state;   /**< State to return to after processing a fenced
                             *   code block.
                             */

bool in_string = false;     /**< Another state variable to avoid
                             * interpreting characters in a string.
                             */
int  fence_indent = 0;      /**< Count of characters in line before fence.
                             *   Remove this number of characters before each
                             *   fenced line before highlighting.
                             */
char fence_char = '\0';     /**< Initial value indicating no current fence in force. */
int  fence_char_count = 0;  /**< Track number of characters in the opening code fence.*/
                             
char fenced_language[10];   /**< Language of fenced area, if designated. */

const HLIndex* g_hlindex = nullptr;

/**
 * @brief Prints the passed char argument to stdout, converting XML-significant
 *        characters to their appropriate entity names.
 *
 * @param c Character to print
 */
void print_char_translated(char c)
{
   switch(c)
   {
      case '@':
         fputs("&commat;", stdout);
         break;
      case '<':
         fputs("&lt;", stdout);
         break;
      case '>':
         fputs("&gt;", stdout);
         break;
      case '&':
         fputs("&amp;", stdout);
         break;
      case '"':
         fputs("&quot;", stdout);
         break;
      case '\'':
         fputs("&apos;", stdout);
         break;
      default:
         fputc(c, stdout);
   }
}

/**
 * @brief Uses print_char_translated to print a string with
 *        converted XML-significant characters.
 *
 * @param str String from which to print
 * @param len Limit of Number of characters to print.
 *
 * This function will print a string to stdout up to the number
 * of characters allowed in the @p len parameter.  This makes it
 * unnecessary to terminate strings with a '\0' as had been done
 * previously.
 */
void print_string_translated(const char *str, int len=2048)
{
   while (len>0 && *str)
   {
      print_char_translated(*str++);
      --len;
   }
}

/**
 * @brief Print up to, but not including char at *end.
 */
void print_to_position(const char *str, const char *end)
{
   while (str>end)
      print_char_translated(*str++);
}

/** Detect if fenced language is set. */
inline bool fence_has_language(void) { return *fenced_language!='\0'; }

/** Compare fenced language to @p str. */
inline bool is_fenced_language(const char *str)
{
   return 0==strcmp(fenced_language, str);
}

void unspecified_fenced_line_func(const char *str)
{
   fputs("*** Error: no Fenced_Line_Func value set. ***\n", stderr);
}

/**
 * @brief Function pointer to allow change behavior of fenced-code line handling.
 */
typedef void (*Fenced_Line_Func)(const char *str);

/** Instantiation of a Fenced_Line_Func pointer. */
Fenced_Line_Func fenced_line_func = unspecified_fenced_line_func;

/**
 * @brief Set or clear the fenced_line_func value.
 *
 * To aid in returning fenced_line_func value to unspecified_fenced_line_func
 * when finished, use this function with a appropriate function, or NULL
 * to restore the error-message-producing function.
 */
inline void set_fenced_line_func(Fenced_Line_Func flf=NULL)
{
   fenced_line_func = flf ? flf : unspecified_fenced_line_func;
}


/** Cast a string to unsigned 16-bit integer for fast comparisons. */
inline uint16_t castui16(const char *str)
{
   return *reinterpret_cast<const uint16_t*>(str);
}

/** Constant value for testing the beginning of lines in a fenced code block. */
const uint16_t asterisk_space = castui16("* ");
/** Constant value to test for end-of-block-comment. */
const uint16_t asterisk_slash = castui16("*/");

/**
 * @brief Function for comparing a string with a uint16_t string constant.
 * @sa @ref castui16
 */
inline bool cmpuint(uint16_t v, const char *s) { return v==castui16(s); }


/**
 * @brief Return matching HLNode if the word is matched in the current HLIndex.
 *
 * The function makes a lower-case copy of the word to compare against the work
 * in the HLIndex.  If a match is made, a pointer to the HLNode is returned.
 *
 * @param  start Address of start of the word.
 * @param  end   Address of the last character in the word.
 * @return Pointer to an HLNode whose tag matches the word.  NULL if not found.
 */
const HLNode *is_highlight_tag(const char *start, const char *end)
{
   // Make lower-case copy in a stack memory block:
   size_t len_of_string = end - start + 1;
   char *word = static_cast<char*>(alloca(len_of_string+1));
   
   char *ptarget = word;
   const char *psource = start;

   while (psource <= end)
   {
      if (*psource>=65 && *psource<=90)
         *ptarget = *psource + 32;
      else
         *ptarget = *psource;

      ++ptarget;
      ++psource;
   }
   *ptarget = '\0';

   // Use lower-case copy to find a node:
   return g_hlindex->seek(word);
}

/**
 * @brief Attempts to find a comment node that matches the beginning of @p s.
 *
 * This function is meant to work on code lines in fenced code blocks.
 *
 * @param s Pointer to a position in a fenced code line.
 * @return Matching HLNode* if found, otherwise NULL;
 */
inline const HLNode* is_fenced_comment_start(const char *s)
{
   return g_hlindex->seek_comment(s);
}

void print_open_element(const char *action)
{
   fputc('<', stdout);
   while (*action)
   {
      if (*action=='.')
      {
         printf(" class=\"%s\"", ++action);
         break;
      }
      else
         fputc(*action, stdout);

      ++action;
   }

   fputc('>', stdout);
}

void print_close_element(const char *action)
{
   fputc('<', stdout);
   fputc('/', stdout);
   
   while (*action && *action!='.')
   {
      fputc(*action, stdout);
      ++action;
   }

   fputc('>', stdout);
   
}

void process_line_comment_line(char *str)
{
   if (*str)
      fputs(str, stdout);
   
   fputc('\n', stdout);
}

void process_block_comment_line(char *str)
{
   // look for the end of the comment block
   
   // if block:
   //    set state  to S_CODE
   //    print up to end-of-comment
   //    hand-off to process_code_line()
   // else
   //    print comment line

   if (str)
      fputs(str, stdout);
   
   fputc('\n', stdout);
}


inline void write_code_start(void){ fputs("  @htmlonly <div class=\"fragment\">\n", stdout); }
inline void write_code_end(void)  { fputs("  </div> @endhtmlonly\n", stdout); }
inline void write_line_start(void){ fputs("  <div class=\"line\">", stdout); }
inline void write_line_end(void)  { fputs("</div>\n", stdout); }

// These might be an alternative if we allow customizing the HTML start and end:
// inline void write_code_start(void){ fputs("  @htmlonly <pre><code>\n", stdout); }
// inline void write_code_end(void)  { fputs("  </code></pre> @endhtmlonly\n", stdout); }
// inline void write_line_start(void){ fputs("  ", stdout); }
// inline void write_line_end(void)  { fputs("\n", stdout); }


/** @brief Print fenced code line as found to let Doxygen interpret later. */
void print_fenced_line_with_doxygen(const char *str)
{
   fputs(str, stdout);
   fputc('\n', stdout);
}

/**
 * @brief Print fenced code line interpreted as simple text line.
 *
 * This function simply wraps the line in an appropriate element (HTML or
 * other) without attempting to differentiate the line type nor any
 * contained keywords.
 *
 * The usual XML entities are replaced.
 */
void print_fenced_line_as_text(const char *str)
{
   write_line_start();
   print_string_translated(str);
   write_line_end();
}

/**
 * @brief Scans line, highlighting words when found in highlight file.
 *
 * This function scans the line for words, the criteria for which is a string
 * of allowed characters surrounded by non-allowed characters.  This is to
 * handle words that are terminated by operators or punctuation, as well as
 * spaces.
 *
 * The function scans for comments matching each non-allowed chartacter
 * against the set of comment strings in the highlighting file.
 */
void print_fenced_line_with_highlighting(const char *str)
{
   assert(g_hlindex);
   
   // Enclose all lines in a div.line element
   write_line_start();

   const HLNode *tagnode;
   const char *tagaction;

   const char *p = str;
   // const char *pstart = nullptr;
   // const char *pend = nullptr;

   while (true)
   {
      if (HLIndex::allowed_in_name(*p))
      {
         tagnode = g_hlindex->seek_word(p);
         if (tagnode)
         {
            const char* tag = tagnode->tag();
            size_t len = strlen(tag);

            if (len)
            {
               tagaction = tagnode->parent()->value();
               print_open_element(tagaction);
               print_string_translated(p, len);
               print_close_element(tagaction);

               p += len;

               continue;
            }
         }
         else   // print to end-of-word:
         {
            while (HLIndex::allowed_in_name(*p))
            {
               print_char_translated(*p);
               ++p;
            }
            // start at top of loop without increment:
            continue;
         }
      }
      else if ((tagnode = is_fenced_comment_start(p)))
      {
         tagaction = tagnode->parent()->value();
         print_open_element(tagaction);
         print_string_translated(p);
         print_close_element(tagaction);

         // Don't bother setting p to the end-of-string,
         // just get out of the loop:
         break;
      }
      else if (*p)
         print_char_translated(*p);

      // This is unusual, but I want to process the '\0' at end of line
      // to ensure that the trailing word is processed:
      if (*p)
         ++p;
      else
         break;
   }

   // Close the div.line element:
   write_line_end();
}




/**
 * @brief Set function pointer for fenced code lines.
 *
 * Compare various languages against the fenced_language value
 * to decide which fenced language line printer to use.
 */
void set_fenced_language_function(void)
{
   if (fence_has_language())
   {
      g_hlindex = HLIndex::get_index(fenced_language);
      if (g_hlindex)
      {
         set_fenced_line_func(print_fenced_line_with_highlighting);
         return;
      }
      else if (is_fenced_language("text") || is_fenced_language("txt"))
      {
         set_fenced_line_func(print_fenced_line_as_text);
         return;
      }

      fprintf(stderr, "*** Unable to find %s.hl. ***\n", fenced_language);
      
      set_fenced_line_func(print_fenced_line_with_doxygen);
   }
}

/** Used to detect need to wrap fenced code. */
inline bool doxygen_is_handling_fenced_code(void)
{
   return fenced_line_func == print_fenced_line_with_doxygen;
}


// void saveline(const char *str)
// {
//    FILE *f = fopen("foutput.txt", "a");
//    if (f)
//    {
//       fputs(str, f);
//       fputc('\n', f);
//       fclose(f);
//    }

// }


/**
 * @brief Looks for closing code fence before passing the string to be highlighted.
 *
 * I make assumptions about fenced code that may not be official but seem to be
 * common practice: that, in addition to the code fences being the same length of
 * the same character, that the opening and closing code fences start on the same
 * column.  This leads to my practice removing from each fenced code line the
 * count of characters before the fence code begins.  This may have to change if
 * this results in a problem with code blocks indented in a list element.
 */
void process_fenced_line(char *str)
{
   // Print empty line if line is shorter than the fence_indent.
   if (strlen(str) <= static_cast<size_t>(fence_indent))
   {
      if (doxygen_is_handling_fenced_code())
         print_fenced_line_with_doxygen(str);
      else
      {
         write_line_start();
         write_line_end();
      }
      return;
   }
   
   // Remove fence indent before any other consideration:
   char *start = str + fence_indent;

   // I'm assuming that if we delete from the beginning of each line
   // the same number of characters that separate the start of the fence
   // tag from the start of the line, it any asterisks that fall after
   // the indent are part of the code, not a comment-block border.
   
   // look for the end of the closing code fence:
   char *end_of_fence = NULL;
   
   char *p = strchr(start, fence_char);
   if (p)
   {
      end_of_fence = p;
      
      int i;
      for (i=0; i<fence_char_count; ++i,++p)
      {
         if (*p != fence_char)
            break;
      }

      // If the count doesn't match, it's not the terminator:
      if (i!=fence_char_count || *p==fence_char)
      {
         end_of_fence = NULL;
      }
   }

   if (end_of_fence)
   {
      if (doxygen_is_handling_fenced_code())
      {
         fputs(start, stdout);
         fputc('\n', stdout);
      }
      else
      {
         // replace fence-line with </div>:
         write_code_end();
      }

      // Nothing should follow an end-of-fence marker, so
      // just change the flag without further processing.
      state = fence_return_state;
      fence_return_state = S_CODE;
      fprintf(stderr, "Reached the end of the fenced code block.\n");
   }
   else
      (*fenced_line_func)(start);
}

/**
 * @brief Set fence values once end of fence string is detected:
 *
 * @return Number of characters to advance the string pointer.
 * @param str Pointer to string just past the last fence character.
 *
 * This function will prepare the global fence-related variables,
 * including fence_indent, fence_char, fence_char_count, and
 * fenced_language.
 *
 * It will compare the indicated language, if found, with what's
 * available (sql only at this point).  
 */
int set_fence_values(const char *fence, int indented)
{
   int advance = 0;
   fence_indent = indented;
   fence_char = *(fence-1);
   fenced_language[0] = '\0';
   fence_return_state = state;
   state = S_FENCED;

   if (*fence=='\0' || isspace(*fence))
   {
      set_fenced_line_func(print_fenced_line_with_doxygen);
      return 0;
   }
   else
   {
      // Use new pointer, preserving start of string for later
      // calculation of number of characters to advance;
      const char *p = fence;
      
      // Use pointer into fenced_language to access memory
      // by incrementing pointer rather than by array index:
      char *l = fenced_language;

      // Check first character to see if the language is brace-enclosed.
      bool braced = *p=='{';

      // Skip brace, if found
      if (braced)
         ++p;
      
      // Skip optional period before extension
      if (*p=='.')
         ++p;

      size_t count = 0;
      
      // Terminate loop while there remains at least one character
      // into which a terminating \0 can be added:
      while (*p && count<sizeof(fenced_language)-1)
      {
         if (braced)
         {
            if (*p=='}')
            {
               *l = '\0';
               advance = 1 + p - fence;
               break;
            }
         }
         else if (isspace(*p))
         {
            *l = '\0';
            advance = p - fence;
            break;
         }

         *l++ = *p++;
         ++count;
      }

      if (count<sizeof(fenced_language))
         *l = '\0';

      if (fenced_language[0])
         set_fenced_language_function();

      if (advance==0)
      {
         // We shouldn't get here, but if the unexpected happens,
         // terminate the language string and return the advance value:
         *l = '\0';
         advance = p - fence;
      }

      return advance;
   }
}

/**
 * @brief Look for a fenced area in a doxy block.
 *
 * Looks for the beginning of a fenced code block.  If no fenced area,
 * it simply prints the buffer and adds a newline at the end (all code lines
 * replace the ending newline with a '\0').
 *
 * If a fenced area is found, the text up to the fence is printed,
 * the state flag is set and then control is handed over to the
 * fenced area function to finish processing the line.
 */
void process_doxy_block_comment_line(char *str)
{
   int findent = 0;
   
   // find first non-space character:
   char *p = str;
   while (*p && isspace(*p))
      ++p;

   // Print out unchanged if empty line:
   if (*p=='\0')
   {
      fputs(str, stdout);
      fputc('\n', stdout);
   }
   else  // if (*p)
   {
      // If at end-of-comment:
      if (cmpuint(asterisk_slash,p))
      {
         // Skip asterisk and slash to continue just after the comment:
         p+=2;
         
         // Print out up to end-of-comment:
         char save = *p;
         *p = '\0';
         fputs(str, stdout);
         *p = save;

         // Revert to regular code processing from here:
         state = S_CODE;
         process_code_line(p);

         // process_code finished the line, so abort continued processing:
         return;
      }
      else if (cmpuint(asterisk_space,p))
         p += 2;
      else if (*p=='*')
         ++p;

      if (!*p)
      {
         fputs(str, stdout);
         fputc('\n', stdout);
      }
      
      // Scan characters for a fence line opening:
      while (*p)
      {
         // Ignoring leading spaces:
         if (!isspace(*p))
         {
            // If a fence character
            if (*p=='`' || *p=='~')
            {
               findent = p - str;
               fence_char = *p;
                  
               // And if at least three of same fence character
               if (fence_char==*(p+1) && fence_char==*(p+2))
               {
                  p += 3;
                  
                  // count and save the number of fence characters:
                  for (fence_char_count=3;
                       *p && *p==fence_char;
                       ++p,++fence_char_count)
                     ;

                  set_fence_values(p, findent);

                  if (doxygen_is_handling_fenced_code())
                  {
                     // Reset fence_indent to 0 to prevent any modification
                     // of comment lines.
                     fence_indent = 0;
                     
                     // print line from start of code fence:
                     fputs(str+fence_indent, stdout);
                     fputc('\n', stdout);
                  }
                  else
                  {
                     fputs("*** Begin fenced code, fencedfilter handling! ***\n", stderr); 
                     // replace fence line with div.fragment element:
                     write_code_start();
                  }

                  // Do not process the rest of this fence line:
                  return;
               }
            }
            
            /*
             * As soon as a non-fence character is found, we stop checking
             * for a fence because the fence opener must be first thing on
             * line (after optional '*' and spaces).
             *
             * We still need to look for the end of an end-of-comment marker
             */
            else
            {
               // Looking for an end-of-comment marker:
               while (*p)
               {
                  // If end-of-comment found:
                  if (cmpuint(asterisk_slash,p))
                  {
                     // Exit XXX_BLOCK_COMMENT state:
                     state = S_CODE;
                     
                     // print up to and including the end-of-comment marker:
                     p += 2;
                     char save = *p;
                     fputs(str, stdout);
                     // Note, no newline here

                     // Then process the line as normal code:
                     *p = save;
                     process_code_line(p);

                     // set flag to skip printing the rest of the line:
                     str = NULL;
                     break;
                  }
                  ++p;
               }

               if (str)
               {
                  fputs(str, stdout);
                  fputc('\n', stdout);
               }

               // Break outer while, too, after finding non-fence character
               break;
            }
         }
         ++p;
         
      } // end of scanning for fence character
   }
}

/**
 * @brief Scan a source file line to find and begin a comment block.
 *
 * This function processes raw lines from an input file.  It looks for the
 * beginning of block comments, both normal (slash-asterisk) and
 * Doxygen-recognized ("slash-asterisk-asterisk-space").
 *
 * Upon finding the block-comment start, it prints the text to that point,
 * sets the state variable to S_BLOCK_COMMENT or S_DOXY_BLOCK_COMMENT and then
 * passes a pointer to the text just following the block start to either
 * process_block_comment_line() or process_doxy_block_comment_line() to finish
 * processing the line.
 */
void process_code_line(char *str)
{
   char *p = str;
   bool escaped = false;

   int count_fence = 0;
   char firstchar = *p;

   if (firstchar=='`' || firstchar=='~')
   {
      while (*p==firstchar)
      {
         ++p;
         ++count_fence;
      }

      if (count_fence>2)
      {
         fence_char_count = count_fence;
         set_fence_values(p, 0);

         if (doxygen_is_handling_fenced_code())
         {
            fputs(str, stdout);
            fputc('\n', stdout);
         }
         else
         {
            write_code_start();
         }

         // Line has already been printed:
         return;
      }

      // Reset pointer if not processing as a fenced code block:
      p = str;

   }

   

   // look for beginning of a comment or a code fence:
   p = str;
   while (*p)
   {
      switch(*p)
      {
         case '\\':
            escaped = !escaped;
            break;
         case '"':
            if (!escaped)
               in_string = !in_string;
            break;
         case '/':
         {
            if (!in_string)
            {
               if (*++p == '*')
               {
                  state = S_BLOCK_COMMENT;
                  ++p;
                  if (*p=='*' || *p=='!')
                  {
                     ++p;

                     // A doxy comment is always /*! or /**, then a space or newline
                     if (!*p || isspace(*p))
                     {
                        state = S_DOXY_BLOCK_COMMENT;
                        fence_char_count = 0;
                     }
                  }

                  if (*p=='\0')
                  {
                     fputs(str, stdout);
                     fputc('\n', stdout);
                     return;
                  }
                  else
                  {
                     char save = *p;
                     *p = '\0';
                     fputs(str, stdout);
                     *p = save;

                     switch(state)
                     {
                        case S_BLOCK_COMMENT:
                           process_block_comment_line(p);
                           return;
                        case S_DOXY_BLOCK_COMMENT:
                           process_doxy_block_comment_line(p);
                           return;
                        default:
                           break;
                     }
                  }
               }
               else
                  // If "/" but not "/*", continue before ++p increment below
                  continue;
            }
            break;
         }
      }
      ++p;
   }

   fputs(str, stdout);
   fputc('\n', stdout);
}


void process_line(char *str)
{
   switch(state)
   {
      case S_CODE:
         process_code_line(str);
         break;
      case S_LINE_COMMENT:
         process_line_comment_line(str);
         break;
      case S_BLOCK_COMMENT:
         process_block_comment_line(str);
         break;
      case S_DOXY_BLOCK_COMMENT:
         process_doxy_block_comment_line(str);
         break;
      case S_FENCED:
         process_fenced_line(str);
         break;
   };
}

void scan(FILE *stream)
{
   char *p;

   while ((p=fgets(scan_buff, sizeof(scan_buff), stream))!=NULL)
   {
      // Replace newline with '\0':
      size_t len = strlen(scan_buff);
      char *p = scan_buff + len - 1;
      if (*p=='\n')
         *p = '\0';
      
      process_line(scan_buff);
   }
}

void test_print_fenced_line_with_highlighting(void)
{
   printf("\nTest print_fenced_line_with_highlighting()\n\n");
   // Call with fake variables to initialize:
   set_fence_values("sql", 0);
   print_fenced_line_with_highlighting("CREATE PROCEDURE IF NOT EXISTS Bozo");
   print_fenced_line_with_highlighting("if (bozo<hoser) then");
   print_fenced_line_with_highlighting("begin");
   print_fenced_line_with_highlighting("   SELECT *");
   print_fenced_line_with_highlighting("     FROM Person;");
   print_fenced_line_with_highlighting("end $$");
}

void load_from_cl(int argc, char **argv)
{
   const char *filename = argv[1];
   FILE *stream = fopen(filename, "r");
   if (stream)
      scan(stream);
   else
      fprintf(stderr, "Unable to open file \"%s\".\n", filename);
}

void show_version(void)
{
   printf("FencedFilter version %d.%02d.\n\n", FF_VERSION_MAJOR, FF_VERSION_MINOR);
}

void show_help(void)
{
   printf("Usage: fencedfilter <filename>\n\n");
}


int main(int argc, char **argv)
{
   if (argc>1)
   {
      if (strcmp(argv[1],"--version")==0)
         show_version();
      else if (strcmp(argv[1],"--help")==0)
         show_help();
      else
         load_from_cl(argc, argv);
   }
   // For debugging, set else if (false) to run test_print_fenced_line_with
   else if (false)
      show_help();
   else
      test_print_fenced_line_with_highlighting();
   
   return 0;
}

