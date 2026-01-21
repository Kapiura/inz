#!/usr/bin/env python3

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
from pathlib import Path
import seaborn as sns

sns.set_style("whitegrid")
plt.rcParams['figure.figsize'] = (12, 8)
plt.rcParams['font.size'] = 10

class ExperimentAnalyzer:
    def __init__(self, data_folder="experiment_data", output_folder="plots"):
        self.data_folder = Path(data_folder)
        self.output_folder = Path(output_folder)
        self.output_folder.mkdir(exist_ok=True)
        
    def load_experiment(self, experiment_name):
        filepath = self.data_folder / f"{experiment_name}.csv"
        if not filepath.exists():
            print(f"Ostrzeżenie: {filepath} nie znaleziono")
            return None
        
        df = pd.read_csv(filepath)
        print(f"Wczytano {experiment_name}: {len(df)} wierszy, {df['run'].nunique()} przebiegów")
        return df
    
    def plot_exp1_threshold_impact(self):
        df = self.load_experiment("exp1_threshold_impact")
        if df is None:
            return
        
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('Eksperyment 1: Wpływ progu zrywania sprężyn', fontsize=16, fontweight='bold')
        
        ax = axes[0, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            threshold = 2.5 + run * 0.5
            ax.plot(run_data['time'], run_data['brokenSprings'],
                    label=f'Próg {threshold:.1f}x', linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Zerwane sprężyny')
        ax.set_title('Zrywanie sprężyn w czasie')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[0, 1]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            threshold = 2.5 + run * 0.5
            ax.plot(run_data['time'], run_data['totalEnergy'],
                    label=f'Próg {threshold:.1f}x', linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Całkowita energia (J)')
        ax.set_title('Rozproszenie energii')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            threshold = 2.5 + run * 0.5
            ax.plot(run_data['time'], run_data['maxTension'],
                    label=f'Próg {threshold:.1f}x', linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Maksymalne naprężenie')
        ax.set_title('Szczytowe naprężenie w czasie')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 1]
        final_broken = []
        thresholds = []
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            final_broken.append(run_data['brokenSprings'].iloc[-1])
            thresholds.append(2.5 + run * 0.5)
        
        ax.bar(range(len(thresholds)), final_broken, alpha=0.7)
        ax.set_xticks(range(len(thresholds)))
        ax.set_xticklabels([f'{t:.1f}x' for t in thresholds])
        ax.set_xlabel('Próg naprężenia')
        ax.set_ylabel('Końcowa liczba zerwanych sprężyn')
        ax.set_title('Łączna liczba zerwań vs próg')
        ax.grid(True, alpha=0.3, axis='y')
        
        plt.tight_layout()
        plt.savefig(self.output_folder / 'exp1_threshold_impact.png', dpi=300, bbox_inches='tight')
        plt.close()
    
    def plot_exp2_wind_strength(self):
        df = self.load_experiment("exp2_wind_strength")
        if df is None:
            return
        
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('Eksperyment 2: Wpływ siły wiatru', fontsize=16, fontweight='bold')
        
        wind_strengths = [0.0, 5.0, 10.0, 15.0, 20.0]
        
        ax = axes[0, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            strength = wind_strengths[run] if run < len(wind_strengths) else run * 5
            ax.plot(run_data['time'], run_data['avgVelocity'],
                    label=f'Wiatr {strength:.1f} m/s', linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Średnia prędkość (m/s)')
        ax.set_title('Ruch tkaniny pod wpływem wiatru')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[0, 1]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            strength = wind_strengths[run] if run < len(wind_strengths) else run * 5
            ax.plot(run_data['time'], run_data['brokenSprings'],
                    label=f'Wiatr {strength:.1f} m/s', linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Zerwane sprężyny')
        ax.set_title('Uszkodzenia strukturalne')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            strength = wind_strengths[run] if run < len(wind_strengths) else run * 5
            ax.plot(run_data['time'], run_data['totalEnergy'],
                    label=f'Wiatr {strength:.1f} m/s', linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Całkowita energia (J)')
        ax.set_title('Transfer energii od wiatru')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 1]
        final_metrics = []
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            final_metrics.append({
                'wind': wind_strengths[run] if run < len(wind_strengths) else run * 5,
                'broken': run_data['brokenSprings'].iloc[-1],
                'energy': run_data['totalEnergy'].mean()
            })
        
        metrics_df = pd.DataFrame(final_metrics)
        x = np.arange(len(metrics_df))
        width = 0.35
        
        ax2 = ax.twinx()
        ax.bar(x - width/2, metrics_df['broken'], width, label='Zerwane sprężyny', alpha=0.7)
        ax2.bar(x + width/2, metrics_df['energy'], width, label='Średnia energia', alpha=0.7)
        
        ax.set_xlabel('Siła wiatru (m/s)')
        ax.set_ylabel('Zerwane sprężyny')
        ax2.set_ylabel('Średnia energia (J)')
        ax.set_xticks(x)
        ax.set_xticklabels([f"{m['wind']:.1f}" for m in final_metrics])
        ax.set_title('Podsumowanie wpływu wiatru')
        ax.legend(loc='upper left')
        ax2.legend(loc='upper right')
        
        plt.tight_layout()
        plt.savefig(self.output_folder / 'exp2_wind_strength.png', dpi=300, bbox_inches='tight')
        plt.close()
    def plot_exp3_wind_direction(self):
        df = self.load_experiment("exp3_wind_direction")
        if df is None:
            return
        
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('Eksperyment 3: Wpływ kierunku wiatru', fontsize=16, fontweight='bold')
        
        directions = ['Prawo', 'Lewo', 'Przód', 'Tył', 'Góra']
        
        ax = axes[0, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            direction = directions[run] if run < len(directions) else f'Kierunek {run}'
            ax.plot(run_data['time'], run_data['brokenSprings'],
                    label=direction, linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Zerwane sprężyny')
        ax.set_title('Zrywanie sprężyn wg kierunku')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[0, 1]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            direction = directions[run] if run < len(directions) else f'Kierunek {run}'
            ax.plot(run_data['time'], run_data['avgTension'],
                    label=direction, linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Średnie naprężenie')
        ax.set_title('Naprężenie systemu wg kierunku')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            direction = directions[run] if run < len(directions) else f'Kierunek {run}'
            ax.plot(run_data['time'], run_data['maxVelocity'],
                    label=direction, linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Maksymalna prędkość (m/s)')
        ax.set_title('Maksymalny ruch wg kierunku')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 1]
        final_comparison = []
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            final_comparison.append(run_data['brokenSprings'].iloc[-1])
        
        ax.bar(range(len(directions)), final_comparison, alpha=0.7)
        ax.set_xticks(range(len(directions)))
        ax.set_xticklabels(directions[:len(final_comparison)], rotation=45)
        ax.set_ylabel('Końcowa liczba zerwanych sprężyn')
        ax.set_title('Uszkodzenia wg kierunku wiatru')
        ax.grid(True, alpha=0.3, axis='y')
        
        plt.tight_layout()
        plt.savefig(self.output_folder / 'exp3_wind_direction.png', dpi=300, bbox_inches='tight')
        plt.close()
    
    def plot_exp4_gravity_impact(self):
        df = self.load_experiment("exp4_gravity_impact")
        if df is None:
            return
        
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('Eksperyment 4: Wpływ grawitacji', fontsize=16, fontweight='bold')
        
        gravity_values = [0.0, -5.0, -9.81, -15.0, -20.0]
        
        ax = axes[0, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            grav = gravity_values[run] if run < len(gravity_values) else -9.81
            ax.plot(run_data['time'], run_data['totalEnergy'],
                    label=f'g={grav:.1f} m/s²', linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Całkowita energia (J)')
        ax.set_title('Energia przy różnej grawitacji')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[0, 1]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            grav = gravity_values[run] if run < len(gravity_values) else -9.81
            ax.plot(run_data['time'], run_data['brokenSprings'],
                    label=f'g={grav:.1f} m/s²', linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Zerwane sprężyny')
        ax.set_title('Tempo uszkodzeń strukturalnych')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            grav = gravity_values[run] if run < len(gravity_values) else -9.81
            ax.plot(run_data['time'], run_data['avgTension'],
                    label=f'g={grav:.1f} m/s²', linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Średnie naprężenie')
        ax.set_title('Poziom naprężeń systemu')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 1]
        summary_data = []
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            grav = abs(gravity_values[run]) if run < len(gravity_values) else 9.81
            summary_data.append({
                'gravity': grav,
                'final_broken': run_data['brokenSprings'].iloc[-1],
                'avg_tension': run_data['avgTension'].mean()
            })
        
        summary_df = pd.DataFrame(summary_data)
        x = np.arange(len(summary_df))
        width = 0.35
        
        ax2 = ax.twinx()
        ax.bar(x - width/2, summary_df['final_broken'], width,
               label='Zerwane sprężyny', alpha=0.7)
        ax2.bar(x + width/2, summary_df['avg_tension'], width,
                label='Średnie naprężenie', alpha=0.7)
        
        ax.set_xlabel('Wartość grawitacji (m/s²)')
        ax.set_ylabel('Zerwane sprężyny')
        ax2.set_ylabel('Średnie naprężenie')
        ax.set_xticks(x)
        ax.set_xticklabels([f"{d['gravity']:.1f}" for d in summary_data])
        ax.set_title('Podsumowanie wpływu grawitacji')
        ax.legend(loc='upper left')
        ax2.legend(loc='upper right')
        
        plt.tight_layout()
        plt.savefig(self.output_folder / 'exp4_gravity_impact.png', dpi=300, bbox_inches='tight')
        plt.close()
    def plot_exp5_cascade_breaking(self):
        df = self.load_experiment("exp5_cascade_breaking")
        if df is None:
            return
        
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('Eksperyment 5: Kaskadowe zrywanie sprężyn', fontsize=16, fontweight='bold')
        
        ax = axes[0, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            ax.plot(run_data['time'], run_data['brokenSprings'],
                    label=f'Przebieg {run+1}', linewidth=2, marker='o', markersize=4)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Skumulowane zerwane sprężyny')
        ax.set_title('Postęp kaskady w czasie')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[0, 1]
        for run in df['run'].unique():
            run_data = df[df['run'] == run].copy()
            run_data['break_rate'] = run_data['brokenSprings'].diff().fillna(0)
            ax.plot(run_data['time'], run_data['break_rate'],
                    label=f'Przebieg {run+1}', linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Zerwane sprężyny na klatkę')
        ax.set_title('Tempo zrywania (intensywność kaskady)')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            ax.plot(run_data['time'], run_data['totalEnergy'],
                    label=f'Przebieg {run+1}', linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Całkowita energia (J)')
        ax.set_title('Utrata energii podczas kaskady')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 1]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            ax.plot(run_data['time'], run_data['maxTension'],
                    label=f'Przebieg {run+1}', linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Maksymalne naprężenie')
        ax.set_title('Szczytowe naprężenia podczas kaskady')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(self.output_folder / 'exp5_cascade_breaking.png', dpi=300, bbox_inches='tight')
        plt.close()
    
    def plot_exp6_solver_stability(self):
        df = self.load_experiment("exp6_solver_stability")
        if df is None:
            return
        
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('Eksperyment 3: Stabilność solvera', fontsize=16, fontweight='bold')
        
        iterations = [3, 5, 10, 15, 20]
        
        ax = axes[0, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            iter_count = iterations[run] if run < len(iterations) else run * 3
            ax.plot(run_data['time'], run_data['totalEnergy'],
                    label=f'{iter_count} iteracji', linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Całkowita energia (J)')
        ax.set_title('Stabilność energii')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[0, 1]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            iter_count = iterations[run] if run < len(iterations) else run * 3
            ax.plot(run_data['time'], run_data['avgTension'],
                    label=f'{iter_count} iteracji', linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Średnie naprężenie')
        ax.set_title('Spełnienie więzów')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            iter_count = iterations[run] if run < len(iterations) else run * 3
            ax.plot(run_data['time'], run_data['fps'],
                    label=f'{iter_count} iteracji', linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('FPS')
        ax.set_title('Wpływ na wydajność')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 1]
        summary = []
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            iter_count = iterations[run] if run < len(iterations) else run * 3
            summary.append({
                'iterations': iter_count,
                'avg_fps': run_data['fps'].mean(),
                'energy_std': run_data['totalEnergy'].std()
            })
        
        summary_df = pd.DataFrame(summary)
        x = np.arange(len(summary_df))
        width = 0.35
        
        ax2 = ax.twinx()
        ax.bar(x - width/2, summary_df['avg_fps'], width,
               label='Średni FPS', alpha=0.7)
        ax2.bar(x + width/2, summary_df['energy_std'], width,
                label='Odchylenie energii', alpha=0.7)
        
        ax.set_xlabel('Liczba iteracji solvera')
        ax.set_ylabel('Średni FPS')
        ax2.set_ylabel('Odchylenie standardowe energii')
        ax.set_xticks(x)
        ax.set_xticklabels([str(s['iterations']) for s in summary])
        ax.set_title('Kompromis wydajność vs stabilność')
        ax.legend(loc='upper left')
        ax2.legend(loc='upper right')
        
        plt.tight_layout()
        plt.savefig(self.output_folder / 'exp6_solver_stability.png', dpi=300, bbox_inches='tight')
        plt.close()
    def plot_exp7_mesh_size(self):
        df = self.load_experiment("exp7_mesh_size_performance")
        if df is None:
            return
        
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('Eksperyment 7: Wpływ rozdzielczości siatki', fontsize=16, fontweight='bold')
        
        mesh_configs = ['15x15', '25x25', '35x35', '50x50']
        
        ax = axes[0, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            config = mesh_configs[run] if run < len(mesh_configs) else f'Siatka {run}'
            ax.plot(run_data['time'], run_data['fps'],
                    label=config, linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('FPS')
        ax.set_title('Wydajność wg rozmiaru siatki')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[0, 1]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            config = mesh_configs[run] if run < len(mesh_configs) else f'Siatka {run}'
            ax.plot(run_data['time'], run_data['totalMasses'],
                    label=config, linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Aktywne masy')
        ax.set_title('Integralność siatki w czasie')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            config = mesh_configs[run] if run < len(mesh_configs) else f'Siatka {run}'
            ax.plot(run_data['time'], run_data['totalEnergy'],
                    label=config, linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Całkowita energia (J)')
        ax.set_title('Dynamika energii wg rozdzielczości')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 1]
        perf_summary = []
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            config = mesh_configs[run] if run < len(mesh_configs) else f'Siatka {run}'
            perf_summary.append({
                'config': config,
                'avg_fps': run_data['fps'].mean(),
                'masses': run_data['totalMasses'].iloc[0]
            })
        
        perf_df = pd.DataFrame(perf_summary)
        x = np.arange(len(perf_df))
        width = 0.35
        
        ax2 = ax.twinx()
        ax.bar(x - width/2, perf_df['avg_fps'], width,
               label='Średni FPS', alpha=0.7)
        ax2.bar(x + width/2, perf_df['masses'], width,
                label='Liczba punktów masy', alpha=0.7)
        
        ax.set_xlabel('Konfiguracja siatki')
        ax.set_ylabel('Średni FPS')
        ax2.set_ylabel('Liczba punktów masy')
        ax.set_xticks(x)
        ax.set_xticklabels(perf_df['config'])
        ax.set_title('Rozdzielczość vs wydajność')
        ax.legend(loc='upper left')
        ax2.legend(loc='upper right')
        
        plt.tight_layout()
        plt.savefig(self.output_folder / 'exp7_mesh_size_performance.png', dpi=300, bbox_inches='tight')
        plt.close()
    
    def plot_exp8_spring_types(self):
        df = self.load_experiment("exp8_spring_types")
        if df is None:
            return
        
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('Eksperyment 4: Porównanie właściwości materiałów', fontsize=16, fontweight='bold')
        
        materials = ['Soft Silk', 'Normal Fabric', 'Heavy Canvas', 'Rubber Sheet']
        
        ax = axes[0, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            material = materials[run] if run < len(materials) else f'Materiał {run}'
            ax.plot(run_data['time'], run_data['totalEnergy'],
                    label=material, linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Całkowita energia (J)')
        ax.set_title('Reakcja energetyczna materiału')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[0, 1]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            material = materials[run] if run < len(materials) else f'Materiał {run}'
            ax.plot(run_data['time'], run_data['avgVelocity'],
                    label=material, linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Średnia prędkość (m/s)')
        ax.set_title('Charakterystyka ruchu')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 0]
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            material = materials[run] if run < len(materials) else f'Materiał {run}'
            ax.plot(run_data['time'], run_data['brokenSprings'],
                    label=material, linewidth=2)
        ax.set_xlabel('Czas (s)')
        ax.set_ylabel('Zerwane sprężyny')
        ax.set_title('Trwałość strukturalna')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        ax = axes[1, 1]
        comparison = []
        for run in df['run'].unique():
            run_data = df[df['run'] == run]
            material = materials[run] if run < len(materials) else f'Materiał {run}'
            comparison.append({
                'material': material,
                'trwałość': 100 - (run_data['brokenSprings'].iloc[-1] /
                                   run_data['totalSprings'].iloc[0] * 100),
                'energia': run_data['totalEnergy'].mean(),
                'ruch': run_data['avgVelocity'].mean()
            })
        
        comp_df = pd.DataFrame(comparison)
        x = np.arange(len(comp_df))
        
        comp_df['trwałość_norm'] = comp_df['trwałość'] / comp_df['trwałość'].max() * 100
        comp_df['energia_norm'] = comp_df['energia'] / comp_df['energia'].max() * 100
        comp_df['ruch_norm'] = comp_df['ruch'] / comp_df['ruch'].max() * 100
        
        width = 0.25
        ax.bar(x - width, comp_df['trwałość_norm'], width, label='Trwałość', alpha=0.8)
        ax.bar(x, comp_df['energia_norm'], width, label='Energia', alpha=0.8)
        ax.bar(x + width, comp_df['ruch_norm'], width, label='Ruch', alpha=0.8)
        
        ax.set_xlabel('Typ materiału')
        ax.set_ylabel('Znormalizowany wynik (0–100)')
        ax.set_title('Porównanie właściwości materiałów')
        ax.set_xticks(x)
        ax.set_xticklabels(comp_df['material'], rotation=15, ha='right')
        ax.legend()
        ax.grid(True, alpha=0.3, axis='y')
        
        plt.tight_layout()
        plt.savefig(self.output_folder / 'exp8_spring_types.png', dpi=300, bbox_inches='tight')
        plt.close()
    def generate_summary_report(self):
        experiments = [
            "exp1_threshold_impact",
            "exp2_wind_strength",
            "exp3_wind_direction",
            "exp4_gravity_impact",
            "exp5_cascade_breaking",
            "exp6_solver_stability",
            "exp7_mesh_size_performance",
            "exp8_spring_types"
        ]
        
        for exp_name in experiments:
            df = self.load_experiment(exp_name)
            if df is not None:
                print(f"\n{exp_name.upper().replace('_', ' ')}")
                print("-" * 60)
                print(f"  Liczba punktów danych: {len(df)}")
                print(f"  Liczba przebiegów: {df['run'].nunique()}")
                print(f"  Zakres czasu: {df['time'].min():.2f}s – {df['time'].max():.2f}s")
                print(f"  Łącznie zerwane sprężyny: {df['brokenSprings'].max():.0f}")
                print(f"  Średni FPS: {df['fps'].mean():.1f}")
                print(f"  Maksymalne zaobserwowane naprężenie: {df['maxTension'].max():.3f}")
        
    def run_all_analyses(self):
        
        self.plot_exp1_threshold_impact()
        self.plot_exp2_wind_strength()
        self.plot_exp3_wind_direction()
        self.plot_exp4_gravity_impact()
        self.plot_exp5_cascade_breaking()
        self.plot_exp6_solver_stability()
        self.plot_exp7_mesh_size()
        self.plot_exp8_spring_types()
        
        self.generate_summary_report()
        

def main():
    analyzer = ExperimentAnalyzer()
    analyzer.run_all_analyses()


if __name__ == "__main__":
    main()
