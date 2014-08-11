using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Microsoft.Win32;

namespace AstVisualizer
{
    /// <summary>
    /// Interaction logic for Main.xaml
    /// </summary>
    public partial class Main : Window
    {
        public Main()
        {
            InitializeComponent();
            m_Source.Text = "int main() { }";
        }

        private async void m_Submit_Click(object sender, RoutedEventArgs e)
        {
            var generator = new AstProducer(m_Source.Text);
            SetAst(await generator.getAst());
        }

        private void m_Load_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog dialog = new OpenFileDialog();
            dialog.Filter = "Test files (*.t)|*.t";
            if (dialog.ShowDialog() ?? false)
            {
                m_Source.Text = File.ReadAllText(dialog.FileName);
                m_Submit_Click(sender, e);
            }
        }

        private void SetAst(AstNode root)
        {
            m_AstTree.Items.Clear();
            m_AstTree.Items.Add(getAst(root));
        }

        private TreeViewItem getAst(AstNode node)
        {
            TreeViewItem ret = new TreeViewItem() { IsExpanded = true };
            ret.Header = node.Content;
            foreach (var child in node.Children)
            {
                ret.Items.Add(getAst(child));
            }
            return ret;
        }
    }
}
