using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace AstVisualizer
{
    class AstProducer
    {
        public AstProducer(string source)
        {
            m_Source = source;
        }

        public async Task<AstNode> getAst()
        {
            var input = Path.GetTempFileName();
            var output = Path.GetTempFileName();
            var resultFile = Path.GetTempFileName();
            File.WriteAllText(input, m_Source);

            // we need to perform the following dance to be able to redirect the stderr to a file
            // (asynchronously read from RedirectStandardError does not interact well with
            // the async pattern below)
            var parser = @"C:\Users\Xin\Documents\GitHubVisualStudio\cppcompiler\compiler\Debug\recog.exe";
            // var parser = @"C:\Users\Xin\Documents\GitHubVisualStudio\cppcompiler\compiler\Debug\parser.exe";
            var arguments = string.Format("/c \"{0} -o {1} {2}\" 2> {3}", parser, output, input, resultFile);
            var result = await getParserResult("cmd.exe", arguments, resultFile);
            return createAst(result);
        }

        private AstNode createAst(string result)
        {
            var lines = result.Split(Environment.NewLine.ToArray(), StringSplitOptions.RemoveEmptyEntries);
            if (lines.Length == 0)
            {
                lines = new string[] { "[Error]" };
            }
            return AstGenerator.get(lines).First();
        }

        private Task<string> getParserResult(string command, string arguments, string resultFile)
        {
            var tcs = new TaskCompletionSource<string>();

            var process = new Process
            {
                StartInfo =
                {
                    FileName = command,
                    Arguments = arguments,
                    CreateNoWindow = true, // do not show the shell window
                    UseShellExecute = false,
                    WindowStyle = ProcessWindowStyle.Hidden,
                },
                EnableRaisingEvents = true
            };

            StringBuilder result = new StringBuilder();
            process.Exited += (sender, args) =>
            {
                tcs.SetResult(File.ReadAllText(resultFile));
                process.Dispose();
            };
            
            process.Start();

            // Note: the following is for debugging only.
            // But I noticed this would trigger a 'completing a task multiple time'
            // exception for unknown reasons.
            // process.WaitForExit();

            return tcs.Task;
        }

        private string m_Source;
    }
}
