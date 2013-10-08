//#include <vld.h>
#include "parser.h"
#include "generator.h"

const std::string ext = "asm";

int main(int argc, char **argv)
{
   if(argc < 2) 
      std::cout << "notFreePascal Compiler, Denis Sushko, 2011" << std::endl;
   else
   {
      FILE *input = _fsopen(argv[2], "r", _SH_DENYWR);
      std::string out_name = "";
      out_name += argv[2];
      for(size_t i = out_name.size() - 1, j = ext.size() - 1; i > out_name.size() - 4; --i, --j)
         out_name[i] = ext[j];
      std::ofstream output(out_name, std::ios::out);
      if (input != nullptr)
      {
         Scanner lexemeScanner(input, output);
         if(strcmp(argv[1], "-l") == 0)
         {
            output << argv[2];
            while (!lexemeScanner.is_eof())
            {
               Token token(lexemeScanner.next());
               token.print(output);
            }
         }
         else if (strcmp(argv[1], "-p") == 0)
         {
            Parser par(lexemeScanner, output);
            par.parse()->print(output);
            par.print_table();
         }
         else if (strcmp(argv[1], "-g") == 0)
         {
            Parser par(lexemeScanner, output);
            auto gen = std::make_shared<Generator>();
            par.generate(gen);
            gen->write_to_file(output, false);
         }
         else if (strcmp(argv[1], "-o") == 0)
         {
            Parser par(lexemeScanner, output);
            auto gen = std::make_shared<Generator>();
            par.generate(gen);
            gen->write_to_file(output, true);
         }
         fclose(input);
         output.close();
      }
      else
         std::cout << "Error opening file" << std::endl;
   }
}