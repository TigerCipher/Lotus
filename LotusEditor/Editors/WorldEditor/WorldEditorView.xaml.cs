using System;
using System.Collections.Generic;
using System.Collections.Specialized;
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
using LotusEditor.GameDev;
using LotusEditor.GameProject;

namespace LotusEditor.Editors
{
    /// <summary>
    /// Interaction logic for WorldEditorView.xaml
    /// </summary>
    public partial class WorldEditorView : UserControl
    {
        public WorldEditorView()
        {
            InitializeComponent();
            Loaded += OnWorldEditorViewLoaded;
        }

        private void OnWorldEditorViewLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnWorldEditorViewLoaded;
            Focus();
            // ((INotifyCollectionChanged)Project.HistoryManager.UndoList).CollectionChanged += (s, e) => Focus();
        }

        private void NewScript_OnButtonClick(object sender, RoutedEventArgs e)
        {
            new NewScriptDialog().ShowDialog();
        }

        private void Build_OnButtonClick(object sender, RoutedEventArgs e)
        {
            
        }
    }
}
