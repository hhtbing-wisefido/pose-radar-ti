"""
v4.0算法功能测试脚本
测试必需命令检测、编码检测、天线配置检测等新功能
"""

import sys
import os

# 添加路径
sys.path.insert(0, os.path.dirname(__file__))

from awrl6844_firmware_matcher import AWRL6844FirmwareMatcher, FirmwareInfo

def test_check_required_commands():
    """测试必需命令检测功能"""
    print("\n" + "="*80)
    print("【测试1】必需命令检测功能")
    print("="*80)
    
    matcher = AWRL6844FirmwareMatcher()
    
    # 测试用例1: 完整的配置文件路径（如果存在）
    test_paths = [
        r"C:\ti\radar_toolbox_3_30_00_06\tools\visualizers\AWRL6844_Incabin_GUI\src\chirpConfigs6844\cpd.cfg",
        r"C:\ti\radar_toolbox_3_30_00_06\tools\visualizers\AWRL6844_Incabin_GUI\src\chirpConfigs6844\sbr.cfg",
        r"C:\ti\radar_toolbox_3_30_00_06\tools\visualizers\AWRL6844_Incabin_GUI\src\chirpConfigs6844\intrusion_detection.cfg",
    ]
    
    for path in test_paths:
        if os.path.exists(path):
            print(f"\n测试文件: {os.path.basename(path)}")
            result = matcher._check_required_commands(path)
            
            print(f"  ✓ 所有必需命令齐全: {result['has_all_required']}")
            if result['missing_commands']:
                print(f"  ✗ 缺少命令: {', '.join(result['missing_commands'])}")
            if result['has_invalid_commands']:
                print(f"  ✗ 无效命令: {', '.join(result['invalid_commands'])}")
            print(f"  → 天线配置方式: {result['antenna_config_mode']}")
        else:
            print(f"\n⚠️ 文件不存在: {path}")

def test_check_file_encoding():
    """测试文件编码检测功能"""
    print("\n" + "="*80)
    print("【测试2】文件编码检测功能")
    print("="*80)
    
    matcher = AWRL6844FirmwareMatcher()
    
    test_paths = [
        r"C:\ti\radar_toolbox_3_30_00_06\tools\visualizers\AWRL6844_Incabin_GUI\src\chirpConfigs6844\cpd.cfg",
    ]
    
    for path in test_paths:
        if os.path.exists(path):
            print(f"\n测试文件: {os.path.basename(path)}")
            result = matcher._check_file_encoding(path)
            
            print(f"  ✓ 纯ASCII编码: {result['is_ascii']}")
            print(f"  ✗ 包含中文: {result['has_chinese']}")
            print(f"  ⚠️ 使用%注释: {result['has_percent_comment']}")
            if result['issue']:
                print(f"  → 问题: {result['issue']}")
        else:
            print(f"\n⚠️ 文件不存在: {path}")

def test_check_antenna_config():
    """测试天线配置检测功能"""
    print("\n" + "="*80)
    print("【测试3】天线配置检测功能")
    print("="*80)
    
    matcher = AWRL6844FirmwareMatcher()
    
    test_paths = [
        r"C:\ti\radar_toolbox_3_30_00_06\tools\visualizers\AWRL6844_Incabin_GUI\src\chirpConfigs6844\cpd.cfg",
    ]
    
    for path in test_paths:
        if os.path.exists(path):
            print(f"\n测试文件: {os.path.basename(path)}")
            result = matcher._check_antenna_config(path)
            
            print(f"  ✗ 使用antGeometryCfg（错误）: {result['uses_antGeometryCfg']}")
            print(f"  ✗ 缺少antGeometryBoard: {result['missing_antGeometryBoard']}")
            print(f"  → 使用手动配置: {result['uses_manual_config']}")
            if result['uses_manual_config']:
                print(f"  → 手动配置完整度: {result['manual_completeness']}/4")
        else:
            print(f"\n⚠️ 文件不存在: {path}")

def test_match_algorithm():
    """测试完整的匹配算法"""
    print("\n" + "="*80)
    print("【测试4】完整匹配算法测试（v4.0）")
    print("="*80)
    
    matcher = AWRL6844FirmwareMatcher()
    
    # 扫描固件和配置文件
    scan_paths = [
        r"C:\ti\radar_toolbox_3_30_00_06",
    ]
    
    for path in scan_paths:
        if os.path.exists(path):
            print(f"\n扫描路径: {path}")
            matcher.scan_directory(path)
            stats = matcher.get_statistics()
            print(f"  应用固件: {stats['application_count']}")
            print(f"  配置文件: {stats['config_count']}")
            
            # 测试第一个固件
            if matcher.application_firmwares:
                test_fw = matcher.application_firmwares[0]
                print(f"\n测试固件: {test_fw.filename}")
                
                cfg_matches = matcher.match_configs_for_firmware(test_fw)
                print(f"\n找到 {len(cfg_matches)} 个配置匹配:")
                
                for i, (cfg, score, validation) in enumerate(cfg_matches[:5]):
                    print(f"\n【匹配{i+1}】{cfg.filename}")
                    
                    # 评分状态
                    if score <= -999999:
                        status = "❌ 不可用"
                    elif score < 0:
                        status = "⚠️ 警告"
                    else:
                        status = "✅ 可用"
                    
                    print(f"  状态: {status}")
                    print(f"  评分: {score:.0f}")
                    
                    # P0级验证
                    print(f"  P0验证:")
                    print(f"    - 必需命令: {'✓' if validation['p0_required_commands'] else '✗'}")
                    print(f"    - 文件编码: {'✓' if validation['p0_encoding'] else '✗'}")
                    print(f"    - 天线配置: {'✓' if validation['p0_antenna'] else '✗'}")
                    print(f"    - 注释格式: {'✓' if validation['p0_comment'] else '✗'}")
                    
                    # P1级评分
                    print(f"  P1评分:")
                    print(f"    - SDK匹配: {validation['p1_sdk']}分")
                    print(f"    - 参数匹配: {validation['p1_params']}分")
                    
                    # 致命错误
                    if validation.get('fatal_errors'):
                        print(f"  致命错误:")
                        for err in validation['fatal_errors']:
                            print(f"    {err}")
                    
                    # 警告信息
                    if validation.get('warnings'):
                        print(f"  警告信息:")
                        for warn in validation['warnings'][:3]:  # 只显示前3条
                            print(f"    {warn}")
                
                break
        else:
            print(f"\n⚠️ 路径不存在: {path}")

def main():
    """运行所有测试"""
    print("="*80)
    print("🧪 v4.0算法功能测试")
    print("="*80)
    
    try:
        # 测试1: 必需命令检测
        test_check_required_commands()
        
        # 测试2: 文件编码检测
        test_check_file_encoding()
        
        # 测试3: 天线配置检测
        test_check_antenna_config()
        
        # 测试4: 完整匹配算法
        test_match_algorithm()
        
        print("\n" + "="*80)
        print("✅ 所有测试完成")
        print("="*80)
        
    except Exception as e:
        print(f"\n❌ 测试失败: {str(e)}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
