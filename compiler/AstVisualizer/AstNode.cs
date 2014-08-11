using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AstVisualizer
{
    class AstNode
    {
        public AstNode(string content) : this(content, new List<AstNode>()) { }

        public AstNode(string content, IEnumerable<AstNode> children)
        {
            Content = content;
            Children = children.ToList();
        }

        public string Content { get; set; }

        public List<AstNode> Children { get; set; }
    }
}
