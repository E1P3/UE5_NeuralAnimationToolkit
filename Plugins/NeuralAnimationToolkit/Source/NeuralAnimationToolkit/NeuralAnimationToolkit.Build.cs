using UnrealBuildTool;

public class NeuralAnimationToolkit : ModuleRules
{
    public NeuralAnimationToolkit(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "UMG",
            "AnimationCore",
            "AnimGraphRuntime",
            "AnimGraph",
            "StructUtils",
            "BlueprintGraph",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects",
            "EditorFramework",
            "UnrealEd",
            "ToolMenus",
            "NNE",
            "Blutility",
            "UMGEditor",
            "ScriptableEditorWidgets",
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "LevelEditor",
                "ToolMenus",
            });
        }
    }
}
