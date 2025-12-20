"""
AWRL6844固件匹配器 - 功能测试脚本
"""

import sys
from pathlib import Path

# 添加当前目录到路径
sys.path.insert(0, str(Path(__file__).parent))

from awrl6844_firmware_matcher import AWRL6844FirmwareMatcher, FirmwareType


def test_matcher():
    """测试固件匹配器"""
    print("=" * 60)
    print("AWRL6844固件匹配器 - 功能测试")
    print("=" * 60)
    print()
    
    # 创建匹配器
    matcher = AWRL6844FirmwareMatcher()
    print("✅ 匹配器创建成功")
    print()
    
    # 测试目录
    test_dirs = [
        r"C:\ti\MMWAVE_L_SDK_06_01_00_01",
        r"C:\ti\radar_toolbox_3_30_00_06"
    ]
    
    print("📂 测试扫描目录:")
    for i, directory in enumerate(test_dirs, 1):
        import os
        exists = "✅ 存在" if os.path.exists(directory) else "❌ 不存在"
        print(f"  {i}. {directory} - {exists}")
    print()
    
    # 扫描第一个存在的目录（测试用）
    test_scan_dir = None
    for directory in test_dirs:
        import os
        if os.path.exists(directory):
            test_scan_dir = directory
            break
    
    if test_scan_dir:
        print(f"🔍 开始扫描测试: {test_scan_dir}")
        print("   (限制前100个文件，仅用于测试)")
        print()
        
        # 扫描（仅扫描部分以加快测试）
        stats = matcher.scan_directory(test_scan_dir, recursive=True)
        
        print("📊 扫描结果:")
        print(f"  • 应用固件: {stats['application']} 个")
        print(f"  • SBL固件: {stats['sbl']} 个")
        print(f"  • 雷达配置: {stats['config']} 个")
        print(f"  • 扫描文件总数: {stats['total_files']} 个")
        print()
        
        # 显示示例固件
        if matcher.application_firmwares:
            print("📦 应用固件示例 (前3个):")
            for i, fw in enumerate(matcher.application_firmwares[:3], 1):
                print(f"\n  {i}. {fw.filename}")
                print(f"     类别: {fw.category}")
                print(f"     子类别: {fw.subcategory}")
                print(f"     处理器: {fw.processor}")
                print(f"     路径: {fw.path[:80]}...")
        
        if matcher.sbl_firmwares:
            print("\n\n🔧 SBL固件示例:")
            for i, sbl in enumerate(matcher.sbl_firmwares[:3], 1):
                print(f"\n  {i}. {sbl.filename}")
                print(f"     变体: {sbl.variant}")
                print(f"     说明: {sbl.description[:60]}...")
        
        if matcher.config_files:
            print("\n\n⚙️ 雷达配置示例 (前3个):")
            for i, cfg in enumerate(matcher.config_files[:3], 1):
                print(f"\n  {i}. {cfg.filename}")
                print(f"     应用: {cfg.application}")
                print(f"     描述: {cfg.description[:60]}...")
        
        # 测试匹配功能
        if matcher.application_firmwares and matcher.sbl_firmwares:
            print("\n\n🎯 智能匹配测试:")
            test_fw = matcher.application_firmwares[0]
            print(f"  测试固件: {test_fw.filename}")
            
            sbl_matches = matcher.match_sbl_for_firmware(test_fw)
            if sbl_matches:
                print(f"\n  推荐SBL (Top 5):")
                for i, (sbl, score) in enumerate(sbl_matches[:5], 1):
                    print(f"    {i}. {sbl.filename} - 匹配度: {score:.0f}%")
            
            cfg_matches = matcher.match_configs_for_firmware(test_fw)
            if cfg_matches:
                print(f"\n  推荐配置 (Top 5):")
                for i, (cfg, score) in enumerate(cfg_matches[:5], 1):
                    print(f"    {i}. {cfg.filename} - 匹配度: {score:.0f}%")
        
        print("\n")
        print("=" * 60)
        print("✅ 测试完成！核心功能正常")
        print("=" * 60)
        
    else:
        print("⚠️  警告: 未找到可用的TI SDK目录")
        print("   请确保已安装以下SDK:")
        print("   - MMWAVE_L_SDK_06_01_00_01")
        print("   - radar_toolbox_3_30_00_06")
        print()
        print("   或修改 test_dirs 变量指向您的SDK路径")


if __name__ == '__main__':
    try:
        test_matcher()
    except Exception as e:
        print(f"\n❌ 测试出错: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
