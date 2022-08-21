using System;
using System.Diagnostics;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media.Animation;

namespace LotusEditor.GameProject
{
    /// <summary>
    /// Interaction logic for ProjectBrowserDialog.xaml
    /// </summary>
    public partial class ProjectBrowserDialog : Window
    {

        private readonly CubicEase _easing = new CubicEase() { EasingMode = EasingMode.EaseInOut };


        public ProjectBrowserDialog()
        {
            InitializeComponent();
            Loaded += OnProjectBrowserDialogLoaded;
        }

        private void OnProjectBrowserDialogLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnProjectBrowserDialogLoaded;
            if (!OpenProject.Projects.Any())
            {
                openProjectButton.IsEnabled = false;
                openProjectView.Visibility = Visibility.Hidden;
                OnToggleButton_Click(createProjectButton, new RoutedEventArgs());
            }
        }

        private void OnToggleButton_Click(object sender, RoutedEventArgs e)
        {
            if (Equals(sender, openProjectButton))
            {
                if (createProjectButton.IsChecked == true)
                {
                    createProjectButton.IsChecked = false;
                    AnimateOpenProject();
                    openProjectView.IsEnabled = true;
                    newProjectView.IsEnabled = false;
                }
                openProjectButton.IsChecked = true;
            }
            else if (Equals(sender, createProjectButton))
            {
                if (openProjectButton.IsChecked == true)
                {
                    openProjectButton.IsChecked = false;
                    AnimateNewProject();
                    openProjectView.IsEnabled = false;
                    newProjectView.IsEnabled = true;
                }
                createProjectButton.IsChecked = true;
            }
        }

        private void AnimateNewProject()
        {
            var highlightAnim = new DoubleAnimation(200, 400, new Duration(TimeSpan.FromSeconds(0.2)));
            highlightAnim.EasingFunction = _easing;
            highlightAnim.Completed += (s, e) =>
            {
                var anim = new ThicknessAnimation(new Thickness(0), new Thickness(-1600, 0, 0, 0), new Duration(TimeSpan.FromSeconds(0.5)))
                    {
                        EasingFunction = _easing
                    };
                browserContent.BeginAnimation(MarginProperty, anim);
            };
            highlightRect.BeginAnimation(Canvas.LeftProperty, highlightAnim);
        }

        private void AnimateOpenProject()
        {
            var highlightAnim = new DoubleAnimation(400, 200, new Duration(TimeSpan.FromSeconds(0.2)));
            highlightAnim.EasingFunction = _easing;
            highlightAnim.Completed += (s, e) =>
            {
                var anim = new ThicknessAnimation(new Thickness(-1600, 0, 0, 0), new Thickness(0), new Duration(TimeSpan.FromSeconds(0.5)))
                    {
                        EasingFunction = _easing
                    };
                browserContent.BeginAnimation(MarginProperty, anim);
            };
            highlightRect.BeginAnimation(Canvas.LeftProperty, highlightAnim);
        }
    }
}
