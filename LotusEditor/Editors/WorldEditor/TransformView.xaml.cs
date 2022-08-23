using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Numerics;
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
using LotusEditor.Components;
using LotusEditor.GameProject;
using LotusEditor.Utility;
using Transform = LotusEditor.Components.Transform;

namespace LotusEditor.Editors
{
    /// <summary>
    /// Interaction logic for TransformView.xaml
    /// </summary>
    public partial class TransformView : UserControl
    {
        private Action _undoAction = null;
        private bool _propertyChanged = false;

        public TransformView()
        {
            InitializeComponent();
            Loaded += OnTransformViewLoaded;
        }

        private void OnTransformViewLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnTransformViewLoaded;
            (DataContext as MSTransform).PropertyChanged += (s, e) => _propertyChanged = true;
        }

        private Action GetAction(Func<Transform, (Transform transform, Vector3)> selector,
    Action<(Transform transform, Vector3)> forEachAction)
        {
            if (!(DataContext is MSTransform vm))
            {
                _undoAction = null;
                _propertyChanged = false;
                return null;
            }

            var selection = vm.SelectedComponents.Select(x => selector(x)).ToList();
            return new Action(() =>
            {
                selection.ForEach(x => forEachAction(x));
                (GameEntityView.Instance.DataContext as MSEntity)?.GetMSComponent<MSTransform>().Refresh();
            });
        }

        private Action GetPositionAction() => GetAction((x) => (x, x.Position), (x) => x.transform.Position = x.Item2);
        private Action GetRotationAction() => GetAction((x) => (x, x.Rotation), (x) => x.transform.Rotation = x.Item2);
        private Action GetScaleAction() => GetAction((x) => (x, x.Scale), (x) => x.transform.Scale = x.Item2);


        private void RecordAction(Action redoAction, string name)
        {
            if (!_propertyChanged) return;
            Debug.Assert(_undoAction != null);
            _propertyChanged = false;
            Project.HistoryManager.AddUndoRedoAction(new UndoRedoAction(name, _undoAction, redoAction));
        }

        private void Position_OnVectorBoxPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            _propertyChanged = false;
            _undoAction = GetPositionAction();
        }

        private void Position_OnVectorBoxPreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            RecordAction(GetPositionAction(), "Position changed");
        }

        private void Position_OnVectorBoxLostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if (!_propertyChanged || _undoAction == null) return;
            Position_OnVectorBoxPreviewMouseLeftButtonUp(sender, null);
        }

        private void Rotation_OnVectorBoxPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            _propertyChanged = false;
            _undoAction = GetRotationAction();
        }

        private void Rotation_OnVectorBoxPreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            RecordAction(GetRotationAction(), "Rotation changed");
        }

        private void Rotation_OnVectorBoxLostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if (!_propertyChanged || _undoAction == null) return;
            Rotation_OnVectorBoxPreviewMouseLeftButtonUp(sender, null);
        }

        private void Scale_OnVectorBoxPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            _propertyChanged = false;
            _undoAction = GetScaleAction();
        }

        private void Scale_OnVectorBoxPreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            RecordAction(GetScaleAction(), "Scale changed");
        }

        private void Scale_OnVectorBoxLostKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            if (!_propertyChanged || _undoAction == null) return;
            Scale_OnVectorBoxPreviewMouseLeftButtonUp(sender, null);
        }
    }
}
