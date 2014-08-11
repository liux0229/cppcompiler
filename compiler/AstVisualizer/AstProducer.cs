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
            File.WriteAllText(input, m_Source);

            var parser = @"C:\PortableDocuments\dev\compiler\cppcompiler\compiler\x64\Debug\recog.exe";
            var arguments = string.Format("-o {0} {1}", output, input);
            var result = await getParserResult(parser, arguments);
            return createAst(result);
        }

        private AstNode createAst(string result)
        {
            var lines = result.Split(Environment.NewLine.ToArray(), StringSplitOptions.RemoveEmptyEntries);
            return AstGenerator.get(lines).First();
        }

        private Task<string> getParserResult(string command, string arguments)
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
                    RedirectStandardError = true,
                    WindowStyle = ProcessWindowStyle.Hidden,
                },
                EnableRaisingEvents = true
            };

            process.Exited += (sender, args) =>
            {
                var result = process.StandardError.ReadToEnd();
                tcs.SetResult(result);
                process.Dispose();
            };

            process.Start();

            return tcs.Task;
        }

        private string m_Source;
    }
}
