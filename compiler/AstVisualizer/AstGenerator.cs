using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AstVisualizer
{
    static class AstGenerator
    {
        public static IEnumerable<AstNode> get(IEnumerable<string> lines)
        {
            return get(new Queue<string>(lines), 0);
        }

        private static IEnumerable<AstNode> get(Queue<string> lines, int level)
        {
            while (lines.Count > 0)
            {
                var line = lines.Peek();
                if (getLevel(line) == level)
                {
                    lines.Dequeue();
                    yield return new AstNode(filter(line), get(lines, level + 1));
                }
                else
                {
                    yield break;
                }
            }
        }

        private static string filter(string line)
        {
            return line.Replace('|', ' ').TrimStart();
        }

        private static int getLevel(string line)
        {
            return line.Count(c => c == '|');
        }
    }
}
